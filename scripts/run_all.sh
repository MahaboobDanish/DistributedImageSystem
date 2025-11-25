#!/bin/bash
# Navigate to project root if not already
cd "$(dirname "$0")/.."

# Create necessary folders
mkdir -p logs images data

echo "Starting Logger..."
tmux new-session -d -s logger "./src/logger/logger"

echo "Starting Processor..."
tmux new-window -t logger:1 -n processor "./src/processor/processor"

echo "Starting Generator..."
tmux new-window -t logger:2 -n generator "./src/generator/generator ./underwater_images"

echo "All apps started in tmux session 'logger'"
echo "Attach with: tmux attach-session -t logger"
