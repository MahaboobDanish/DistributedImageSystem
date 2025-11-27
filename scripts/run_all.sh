#!/bin/bash

cd "$(dirname "$0")/.."
ROOT_DIR=$(pwd)
CONFIG_FILE="$ROOT_DIR/config/default_config.json"

# --- sanity checks ---
if [ ! -f "$CONFIG_FILE" ]; then
  echo "ERROR: Config file not found at $CONFIG_FILE"
  exit 1
fi

if [ ! -x "$ROOT_DIR/build/src/logger/logger" ]; then
  echo "ERROR: Logger binary not found. Build the project first."
  exit 1
fi

# --- read config ---
GEN_FOLDER=$(jq -r '.generator.image_folder' "$CONFIG_FILE")
GEN_PORT=$(jq -r '.generator.publish_port' "$CONFIG_FILE")

PROC_SUB_PORT=$(jq -r '.processor.subscribe_port' "$CONFIG_FILE")
PROC_PUB_PORT=$(jq -r '.processor.publish_port' "$CONFIG_FILE")

LOGGER_SUB_PORT=$(jq -r '.logger.subscribe_port' "$CONFIG_FILE")
LOGGER_SAVE_PATH=$(jq -r '.logger.image_save_path' "$CONFIG_FILE")
DB_PATH=$(jq -r '.logger.db_path' "$CONFIG_FILE")

LOG_FOLDER=$(jq -r '.logging.log_folder' "$CONFIG_FILE")

# --- derive db directory safely ---
DB_DIR=$(dirname "$DB_PATH")

# --- create ONLY what config defines ---
mkdir -p "$GEN_FOLDER"
mkdir -p "$LOGGER_SAVE_PATH"
mkdir -p "$LOG_FOLDER"
mkdir -p "$DB_DIR"


# --- start tmux cleanly ---
echo "Starting Logger..."
tmux new-session -d -s logger
tmux send-keys -t logger:0 \
"cd $ROOT_DIR && ./build/src/logger/logger $LOGGER_SUB_PORT $LOGGER_SAVE_PATH | tee $LOG_FOLDER/logger.log" C-m

sleep 2
echo "Starting Processor..."
tmux new-window -t logger:1
tmux send-keys -t logger:1 \
"cd $ROOT_DIR && ./build/src/processor/processor $PROC_SUB_PORT $PROC_PUB_PORT | tee $LOG_FOLDER/processor.log" C-m

sleep 2
echo "Starting Generator..."
tmux new-window -t logger:2
tmux send-keys -t logger:2 \
"cd $ROOT_DIR && ./build/src/generator/generator $GEN_FOLDER $GEN_PORT | tee $LOG_FOLDER/generator.log" C-m

echo "======================================"
echo "All apps started."
echo "Attach with: tmux attach -t logger"
echo "======================================"


######################################
# #!/bin/bash

# # Navigate to project root
# cd "$(dirname "$0")/.."
# ROOT_DIR=$(pwd)

# # Check if tmux session exists
# if tmux has-session -t logger 2>/dev/null; then
#     echo "Tmux session 'logger' already exists. Attach using:"
#     echo "tmux attach-session -t logger"
#     exit 0
# fi

# # Create necessary folders
# mkdir -p logs images data

# echo "Starting Logger..."
# tmux new-session -d -s logger "bash -c '$ROOT_DIR/build/src/logger/logger 2>&1 | tee $ROOT_DIR/logs/logger.log'"

# sleep 1
# echo "Starting Processor..."
# tmux new-window -t logger:1 -n processor "bash -c '$ROOT_DIR/build/src/processor/processor 2>&1 | tee $ROOT_DIR/logs/processor.log'"

# sleep 1
# echo "Starting Generator..."
# tmux new-window -t logger:2 -n generator "bash -c '$ROOT_DIR/build/src/generator/generator $ROOT_DIR/underwater_images 2>&1 | tee $ROOT_DIR/logs/generator.log'"

# echo "All apps started in tmux session 'logger'"
# echo "Attach with: tmux attach-session -t logger"

##########################################################

# #!/bin/bash

# # Navigate to project root
# cd "$(dirname "$0")/.."
# ROOT_DIR=$(pwd)

# # Check if tmux session exists
# if tmux has-session -t logger 2>/dev/null; then
#     echo "Tmux session 'logger' already exists. Attach using:"
#     echo "tmux attach-session -t logger"
#     exit 0
# fi

# # Create necessary folders
# mkdir -p logs images data

# echo "Starting Logger..."
# tmux new-session -d -s logger "bash -c '$ROOT_DIR/src/logger/logger 2>&1 | tee $ROOT_DIR/logs/logger.log'"

# sleep 1
# echo "Starting Processor..."
# tmux new-window -t logger:1 -n processor "bash -c '$ROOT_DIR/src/processor/processor 2>&1 | tee $ROOT_DIR/logs/processor.log'"

# sleep 1
# echo "Starting Generator..."
# tmux new-window -t logger:2 -n generator "bash -c '$ROOT_DIR/src/generator/generator $ROOT_DIR/underwater_images 2>&1 | tee $ROOT_DIR/logs/generator.log'"

# echo "All apps started in tmux session 'logger'"
# echo "Attach with: tmux attach-session -t logger"

######################################################

# #!/bin/bash
# # Navigate to project root if not already
# cd "$(dirname "$0")/.."

# # Create necessary folders
# mkdir -p logs images data

# echo "Starting Logger..."
# tmux new-session -d -s logger "./src/logger/logger"

# echo "Starting Processor..."
# tmux new-window -t logger:1 -n processor "./src/processor/processor"

# echo "Starting Generator..."
# tmux new-window -t logger:2 -n generator "./src/generator/generator ./underwater_images"

# echo "All apps started in tmux session 'logger'"
# echo "Attach with: tmux attach-session -t logger"
