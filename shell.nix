{ pkgs ? import <nixpkgs> { config.allowUnfree = true; } }:

let
  # Скрипт для настройки DISPLAY
  setupDisplay = pkgs.writeScriptBin "setup-display" ''
    #!/usr/bin/env bash
    
    function find_working_display() {
      for ip in "127.0.0.1" "localhost" "172.21.96.1" "10.116.3.29"; do
        export DISPLAY="$ip:0"
        if xdpyinfo >/dev/null 2>&1; then
          echo "$ip"
          return 0
        fi
      done
      echo ""
      return 1
    }

    WORKING_IP=$(find_working_display)
    if [ ! -z "$WORKING_IP" ]; then
      echo "$WORKING_IP"
      exit 0
    fi
    echo ""
    exit 1
  '';

in pkgs.mkShell {
  buildInputs = with pkgs; [
    cmake
    xorg.libX11
    xorg.libX11.dev
    xorg.libXext
    xorg.libXrender
    xorg.libXfixes
    xorg.xorgproto
    xorg.xhost
    xorg.xdpyinfo
    libGL
    libGLU
    mesa
    mesa.drivers
    libglvnd
    egl-wayland
    wayland
    wayland-protocols
    xorg.xeyes
    setupDisplay
    vscode
  ];

  shellHook = ''
    # Настраиваем переменные окружения для OpenGL
    export LIBGL_DEBUG=verbose
    export MESA_DEBUG=1
    export EGL_LOG_LEVEL=debug
    export LIBGL_ALWAYS_INDIRECT=1
    
    echo "OpenGL development environment ready!"
    echo "Testing X server connection..."
    
    # Находим работающий IP для X сервера
    WORKING_IP=$(${setupDisplay}/bin/setup-display)
    if [ ! -z "$WORKING_IP" ]; then
      export DISPLAY="$WORKING_IP:0"
      echo "Successfully connected to X server at $DISPLAY"
    else
      echo "WARNING: Could not connect to X server!"
      echo "Please make sure VcXsrv is running with correct settings"
    fi

    echo ""
    echo "Make sure VcXsrv is running with:"
    echo "- Multiple windows"
    echo "- Display number: 0"
    echo "- Start no client"
    echo "- Disable access control (checked)"
    echo "- Native opengl (checked)"
    echo ""
    echo "Current DISPLAY=$DISPLAY"
    echo ""
    echo "To test X11 connection, run: xeyes"
  '';
}