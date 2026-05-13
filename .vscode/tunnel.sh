#!/usr/bin/env bash
# =============================================================================
# VS Code Remote Tunnel Setup — Ubuntu 22.04+
# =============================================================================
# Safe to run multiple times (idempotent).
# Run as your normal user (NOT root). Will sudo when needed.
# =============================================================================

set -euo pipefail

# ── Colours and helpers (defined first so validation can use them) ─────────────
RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'
CYAN='\033[0;36m'; BOLD='\033[1m'; RESET='\033[0m'

info()    { echo -e "${CYAN}[INFO]${RESET}  $*"; }
ok()      { echo -e "${GREEN}[OK]${RESET}    $*"; }
warn()    { echo -e "${YELLOW}[WARN]${RESET}  $*"; }
die()     { echo -e "${RED}[ERROR]${RESET} $*" >&2; exit 1; }
header()  { echo -e "\n${BOLD}══ $* ══${RESET}"; }

usage() {
    cat <<EOF
Usage: $(basename "$0") [options]

Sets up a VS Code Remote Tunnel so you can edit from a browser or tablet
via https://vscode.dev/tunnel/<name>.

Options:
  -h, --help      Show this help and exit
  -m, --mode <n>  Run mode: 1 = systemd service (default), 2 = interactive foreground
  -n, --name <s>  Tunnel name (overrides VSCODE_TUNNEL_NAME env var and hostname)

Environment variables (alternative to flags):
  VSCODE_TUNNEL_NAME   Tunnel name (letters, digits, hyphens, underscores; max 60 chars)
  MODE                 Run mode (1 or 2)

Examples:
  # First-time setup — interactive so you can complete the OAuth flow
  $(basename "$0") --mode 2 --name mydevbox

  # Subsequent runs — install as a persistent background service
  $(basename "$0") --mode 1 --name mydevbox

Tunnel URL pattern:
  https://vscode.dev/tunnel/<name>

Service management (after mode 1):
  systemctl --user status  vscode-tunnel
  journalctl --user -u     vscode-tunnel -f
  systemctl --user stop    vscode-tunnel
  systemctl --user restart vscode-tunnel
  systemctl --user disable --now vscode-tunnel

Full removal:
  systemctl --user disable --now vscode-tunnel
  rm -f ~/.config/systemd/user/vscode-tunnel.service
  sudo loginctl disable-linger \$USER
EOF
    exit 0
}

# ── Parse arguments ────────────────────────────────────────────────────────────
while [[ $# -gt 0 ]]; do
    case "$1" in
        -h|--help)   usage ;;
        -m|--mode)   MODE="${2:?--mode requires a value (1 or 2)}"; shift 2 ;;
        -n|--name)   VSCODE_TUNNEL_NAME="${2:?--name requires a value}"; shift 2 ;;
        *) die "Unknown option: $1  (run with --help for usage)" ;;
    esac
done

# ── Configuration ─────────────────────────────────────────────────────────────
# Tunnel name becomes part of the URL — restrict to safe characters only.
_raw_name="${VSCODE_TUNNEL_NAME:-$(hostname -s)}"
TUNNEL_NAME="${_raw_name//[^a-zA-Z0-9_-]/}"
if [[ -z "$TUNNEL_NAME" ]]; then
    die "TUNNEL_NAME is empty after stripping unsafe characters (raw: '${_raw_name}').
    Set VSCODE_TUNNEL_NAME to a string containing only letters, digits, hyphens, or underscores."
fi
# Truncate after the empty check so the error message fires on a name that
# becomes empty before truncation, not after. Warn when truncation occurs so
# two long hostnames that collapse to the same 60-char prefix are visible.
if [[ "${#TUNNEL_NAME}" -gt 60 ]]; then
    TUNNEL_NAME="${TUNNEL_NAME:0:60}"
    warn "TUNNEL_NAME truncated to 60 characters: '${TUNNEL_NAME}'
    If this collides with another machine, set VSCODE_TUNNEL_NAME to a unique short name."
fi
if [[ "$TUNNEL_NAME" != "$_raw_name" && "${#_raw_name}" -le 60 ]]; then
    warn "TUNNEL_NAME sanitised: '${_raw_name}' → '${TUNNEL_NAME}'"
fi
SERVICE_NAME="vscode-tunnel"
EXTENSIONS=(
    "ms-vscode.cpptools"
    "ms-vscode.cmake-tools"
)

# =============================================================================
# 1. PREFLIGHT — verify VS Code + code CLI
# =============================================================================
header "Preflight checks"

if ! command -v code &>/dev/null; then
    die "'code' CLI not found. Install VS Code from https://code.visualstudio.com/download
    and ensure the 'code' command is in your PATH (VS Code installer does this automatically)."
fi
ok "VS Code CLI found: $(code --version | head -1)"

for _cmd in systemctl loginctl; do
    command -v "$_cmd" &>/dev/null || die "'${_cmd}' not found. This script requires systemd (Ubuntu 22.04+)."
done
_user_state="$(systemctl --user is-system-running 2>/dev/null || true)"
case "$_user_state" in
    running|degraded) ok "systemd user manager is running (state: ${_user_state})" ;;
    *) warn "systemd user manager state: '${_user_state:-unavailable}'. Service mode may fail.
    Try: systemctl --user status" ;;
esac

# =============================================================================
# 2. DEPENDENCIES — qrencode for QR codes in the terminal (optional)
# =============================================================================
header "Dependencies"

_have_qrencode=0
if command -v qrencode &>/dev/null; then
    _have_qrencode=1
else
    info "Installing qrencode for QR display (optional — plain URL always shown)…"
    if sudo apt-get update -qq && sudo apt-get install -y -qq qrencode; then
        _have_qrencode=1
        ok "qrencode installed"
    else
        warn "Could not install qrencode — QR codes will be skipped. The plain URL will still be shown."
    fi
fi

# =============================================================================
# 3. EXTENSIONS — install / skip if already present
# =============================================================================
header "VS Code extensions"

INSTALLED=$(code --list-extensions 2>/dev/null) || { warn "Could not list extensions; will attempt install of all."; INSTALLED=""; }
for ext in "${EXTENSIONS[@]}"; do
    if [[ -n "$INSTALLED" ]] && echo "$INSTALLED" | grep -qixF "$ext"; then
        ok "Already installed: $ext"
    else
        info "Installing $ext …"
        code --install-extension "$ext"
        ok "Installed: $ext"
    fi
done

# =============================================================================
# 4. SLEEP PREVENTION — scoped to the tunnel process, not the whole machine
# =============================================================================
# Interactive mode: systemd-inhibit wraps the code process (removed when it exits).
# Service mode:     the unit file uses systemd-inhibit in ExecStart.
# No system-wide sleep policy files are written.
header "Sleep / suspend prevention"
info "Sleep inhibition is scoped per-process via systemd-inhibit (no machine-wide changes)"

# =============================================================================
# 5. TUNNEL URL — known ahead of time because we fix --name
# =============================================================================
TUNNEL_URL="https://vscode.dev/tunnel/${TUNNEL_NAME}"

print_qr() {
    echo ""
    if [[ "$_have_qrencode" -eq 1 ]]; then
        echo -e "${BOLD}Scan to open in your browser / tablet:${RESET}"
        qrencode -t ansiutf8 "$TUNNEL_URL" || true
    fi
    echo -e "${BOLD}URL:${RESET} ${CYAN}${TUNNEL_URL}${RESET}"
    echo ""
}

# =============================================================================
# 6. RESOLVE BINARIES — needed by both modes
# =============================================================================
CODE_BIN="$(command -v code)"
if ! INHIBIT_BIN="$(command -v systemd-inhibit 2>/dev/null)"; then
    die "systemd-inhibit not found. Is systemd installed?"
fi

UNIT_DIR="$HOME/.config/systemd/user"
UNIT_FILE="$UNIT_DIR/${SERVICE_NAME}.service"

# =============================================================================
# 7. START / RESTART the service or run interactively
# =============================================================================
header "Starting tunnel"

# Probe linger state without mutating anything.
# _linger_active: linger is already on — service will survive logout now.
# _linger_can_enable: passwordless sudo available — we can enable it when the
#   user picks service mode, but haven't done so yet.
_linger_active=0
_linger_can_enable=0
if loginctl show-user "$USER" 2>/dev/null | grep -q "Linger=yes"; then
    _linger_active=1
elif sudo -n true 2>/dev/null; then
    _linger_can_enable=1
fi

if [[ "$_linger_active" -eq 1 ]]; then
    _svc_label="Systemd service  (auto-start on login, survives logout/reboot)"
elif [[ "$_linger_can_enable" -eq 1 ]]; then
    _svc_label="Systemd service  (auto-start on login, will be configured to survive logout)"
else
    _svc_label="Systemd service  (auto-start on login; stops on logout — enable with: sudo loginctl enable-linger ${USER})"
fi

# Allow non-interactive callers to set MODE in the environment and skip the prompt.
if [[ -n "${MODE:-}" ]]; then
    if [[ "$MODE" != "1" && "$MODE" != "2" ]]; then
        die "MODE=${MODE} is invalid. Set MODE=1 (systemd service) or MODE=2 (interactive)."
    fi
    info "Using MODE=${MODE} from environment (skipping prompt)."
elif [[ ! -t 0 ]]; then
    MODE="1"
    info "Non-interactive invocation detected — defaulting to MODE=1 (systemd service)."
    info "Set MODE=2 in the environment to force interactive mode."
else
    echo -e "Choose how to run the tunnel:"
    echo -e "  ${BOLD}1)${RESET} ${_svc_label}"
    echo -e "  ${BOLD}2)${RESET} Interactive       (foreground, good for first-time auth)"
    echo ""
    read -rp "Enter 1 or 2 [default: 1]: " MODE < /dev/tty
    MODE="${MODE:-1}"
    if [[ "$MODE" != "1" && "$MODE" != "2" ]]; then
        warn "Invalid input '${MODE}' — defaulting to mode 1 (systemd service)."
        MODE="1"
    fi
fi

if [[ "$MODE" == "2" ]]; then
    # ── Interactive mode ───────────────────────────────────────────────────
    echo ""
    info "Starting tunnel interactively. Follow the auth prompt below."
    echo -e "${YELLOW}After authorising, the tunnel will be live at:${RESET}"
    print_qr
    echo -e "${YELLOW}Press Ctrl-C to stop.${RESET}\n"
    exec "$INHIBIT_BIN" --what=sleep:idle --who=vscode-tunnel --why=vscode-tunnel \
        "$CODE_BIN" tunnel --accept-server-license-terms --name "$TUNNEL_NAME"
else
    # ── Systemd service mode ───────────────────────────────────────────────
    # Write or update the unit now that we know service mode was chosen.
    mkdir -p "$UNIT_DIR"
    for _bin_check in "$CODE_BIN" "$INHIBIT_BIN"; do
        if [[ "$_bin_check" == *" "* ]]; then
            die "Binary path contains spaces and cannot be used in a systemd unit: '${_bin_check}'
    Create a symlink in a space-free location and rerun:
      sudo ln -s '${_bin_check}' /usr/local/bin/$(basename "${_bin_check}")"
        fi
    done
    NEW_UNIT=$(cat <<EOF
[Unit]
Description=VS Code Remote Tunnel
After=network-online.target
Wants=network-online.target

[Service]
Type=simple
ExecStart=${INHIBIT_BIN} --what=sleep:idle --who=vscode-tunnel --why=vscode-tunnel \\
    ${CODE_BIN} tunnel --accept-server-license-terms --name ${TUNNEL_NAME}
Restart=on-failure
RestartSec=10
Environment=HOME=%h

[Install]
WantedBy=default.target
EOF
)
    OLD_UNIT=""
    [[ -f "$UNIT_FILE" ]] && OLD_UNIT="$(cat "$UNIT_FILE")"
    if [[ "$NEW_UNIT" != "$OLD_UNIT" ]]; then
        printf '%s\n' "$NEW_UNIT" > "$UNIT_FILE"
        systemctl --user daemon-reload
        ok "Service unit updated: $UNIT_FILE"
    else
        ok "Service unit unchanged"
    fi

    if [[ "$_linger_active" -eq 1 ]]; then
        ok "Linger already enabled (service survives logout)"
    elif [[ "$_linger_can_enable" -eq 1 ]]; then
        sudo loginctl enable-linger "$USER"
        ok "Linger enabled (service survives logout)"
    else
        warn "Could not enable linger (no passwordless sudo). The tunnel will stop on logout."
        warn "To enable manually: sudo loginctl enable-linger ${USER}"
    fi

    if systemctl --user is-active --quiet "${SERVICE_NAME}"; then
        info "Service already running — restarting…"
        systemctl --user restart "${SERVICE_NAME}"
    else
        systemctl --user enable --now "${SERVICE_NAME}"
    fi

    _started=0
    for _i in {1..20}; do
        if systemctl --user is-active --quiet "${SERVICE_NAME}"; then
            _started=1; break
        fi
        sleep 0.5
    done
    if [[ "$_started" -eq 1 ]]; then
        ok "Service is running"
    else
        warn "Service did not become active within 10 s. Check: journalctl --user -u ${SERVICE_NAME} -f"
    fi

    echo ""
    echo -e "${YELLOW}NOTE: On first run you must authorise once. Check the journal:${RESET}"
    echo -e "  ${CYAN}journalctl --user -u ${SERVICE_NAME} -f${RESET}"
    echo ""
    print_qr
fi

# =============================================================================
# 8. SUMMARY
# =============================================================================
header "Done"

cat <<EOF
Tunnel name : ${TUNNEL_NAME}
Tunnel URL  : ${TUNNEL_URL}

── Service management ───────────────────────────────────────────
  Status  : systemctl --user status  ${SERVICE_NAME}
  Logs    : journalctl --user -u ${SERVICE_NAME} -f
  Stop    : systemctl --user stop    ${SERVICE_NAME}
  Restart : systemctl --user restart ${SERVICE_NAME}
  Disable : systemctl --user disable --now ${SERVICE_NAME}

── To fully remove everything ────────────────────────────────────
  systemctl --user disable --now ${SERVICE_NAME}
  rm -f ${UNIT_FILE}
  sudo loginctl disable-linger ${USER}   # undo linger if no longer wanted

── Troubleshooting ──────────────────────────────────────────────
  • "Tunnel not found"   — run interactively first to complete OAuth
  • Connection refused   — check: systemctl --user status ${SERVICE_NAME}
  • Extensions missing   — rerun this script; it installs missing ones only
  • Machine still sleeps — confirm systemd-inhibit is in ExecStart:
                           systemctl --user cat ${SERVICE_NAME}
  • Port 443 blocked     — VS Code tunnels use HTTPS outbound; check firewall
EOF
