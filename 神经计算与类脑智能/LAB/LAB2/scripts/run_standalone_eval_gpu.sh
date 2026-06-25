#!/bin/bash

if [ $# != 2 ]; then
  echo "Usage: bash run_standalone_eval_gpu.sh [DATA_PATH] [CKPT_FILE]"
  exit 1
fi

SCRIPT_DIR=$(cd "$(dirname "$0")" || exit 1; pwd)
PROJECT_DIR=$(cd "${SCRIPT_DIR}/.." || exit 1; pwd)

python "${PROJECT_DIR}/eval.py" \
  --device_target GPU \
  --data_path "$1" \
  --ckpt_path "$2"
