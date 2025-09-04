#!/bin/bash

# Install tmux if not present
./install_tmux.sh

# Compile the server and client
echo "Compiling server and client..."
gcc chat_server.c -o server -pthread
gcc chat_client.c -o client -pthread

# Check if compilation was successful
if [ ! -f "./server" ] || [ ! -f "./client" ]; then
    echo "Compilation failed!"
    exit 1
fi

# Create a new tmux session named "chat"
tmux new-session -d -s chat

# Split the window into three panes
tmux split-window -v -t chat
tmux split-window -h -t chat:0.0

# Pane 0: Banner
tmux send-keys -t chat:0.0 'clear' C-m
tmux send-keys -t chat:0.0 'echo "Banner Pane"' C-m
tmux send-keys -t chat:0.0 './server' C-m

# Pane 1: Client list (will show connected clients)
tmux send-keys -t chat:0.1 'echo "Client Connection Pane"' C-m
tmux send-keys -t chat:0.1 'watch -n 1 "netstat -an | grep :8080"' C-m

# Pane 2: Client interface
tmux send-keys -t chat:0.2 'echo "Client Chat Pane"' C-m
tmux send-keys -t chat:0.2 'sleep 2 && ./client' C-m

# Adjust pane sizes
tmux select-pane -t chat:0.0
tmux resize-pane -U 10
tmux select-pane -t chat:0.2
tmux resize-pane -U 5

# Attach to the tmux session
tmux attach-session -t chat
