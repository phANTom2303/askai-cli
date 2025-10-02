#!/bin/bash

# --- Installation Script for askai ---

# Stop the script if any command fails
set -e

TOOL_NAME="askai"
INSTALL_DIR="$HOME/.local/bin" # A standard location for user binaries
BASHRC_FILE="$HOME/.bashrc"

echo "Starting installation for $TOOL_NAME..."

# 1. Build the C program using the Makefile
echo "Compiling the source code..."
make clean
make

# 2. Create the installation directory if it doesn't exist
echo "Ensuring installation directory exists at $INSTALL_DIR..."
mkdir -p "$INSTALL_DIR"

# 3. Move the executable to the installation directory
echo "Installing executable to $INSTALL_DIR..."
mv "$TOOL_NAME" "$INSTALL_DIR/"

# 4. Add the tool's directory to the PATH in .bashrc if it's not already there
# This makes the command available globally in your terminal.
EXPORT_LINE="export PATH=\"$INSTALL_DIR:\$PATH\""

echo "Configuring environment..."
if ! grep -qF "PATH=\"$INSTALL_DIR:\$PATH\"" "$BASHRC_FILE"; then
    echo "Adding $TOOL_NAME to PATH in $BASHRC_FILE"
    echo "" >> "$BASHRC_FILE" # Add a newline for spacing
    echo "# Add $TOOL_NAME directory to PATH" >> "$BASHRC_FILE"
    echo "$EXPORT_LINE" >> "$BASHRC_FILE"
else
    echo "$TOOL_NAME directory is already configured in your PATH."
fi

echo ""
echo "Installation complete!"
echo "To begin using the 'askai' command, please run the following or open a new terminal:"
echo "source $BASHRC_FILE"