pipeline {
  agent { label 'build' }
  options { timestamps() }
  environment {
    GITHUB_CREDS = credentials('github')
  }

  stages {
    stage('Setup') {
      steps {
        sh '''
          set -eu
          if command -v python3 >/dev/null 2>&1; then
            python3 -m pip install --upgrade pip platformio intelhex || python3 -m pip install --user --upgrade pip platformio intelhex
          elif command -v python >/dev/null 2>&1; then
            python -m pip install --upgrade pip platformio intelhex || python -m pip install --user --upgrade pip platformio intelhex
          else
            echo "Python not found on build agent" >&2
            exit 1
          fi
        '''
      }
    }

    stage('Build') {
      steps {
        sh '''
          set -eu
          git config --global url."https://x-access-token:${GITHUB_CREDS_PSW}@github.com/".insteadOf "https://github.com/"
          if command -v platformio >/dev/null 2>&1; then
            PIO=platformio
          elif [ -x "$HOME/.local/bin/platformio" ]; then
            PIO="$HOME/.local/bin/platformio"
          else
            USER_BASE=$(python3 -m site --user-base 2>/dev/null || python -m site --user-base)
            PIO="$USER_BASE/bin/platformio"
          fi

          "$PIO" run -e esp32dev_serial
        '''
      }
    }
  }
}
