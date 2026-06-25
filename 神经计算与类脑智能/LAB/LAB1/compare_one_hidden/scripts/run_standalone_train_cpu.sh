#!/bin/bash

if [ $# != 2 ]; then
  echo "Usage: bash run_standalone_train_cpu.sh [DATA_PATH] [CKPT_PATH]"
  exit 1
fi

SCRIPT_DIR=$(cd "$(dirname "$0")" || exit 1; pwd)
PROJECT_DIR=$(cd "${SCRIPT_DIR}/.." || exit 1; pwd)

python "${PROJECT_DIR}/train.py" \
  --device_target CPU \
  --data_path "$1" \
  --ckpt_path "$2"
