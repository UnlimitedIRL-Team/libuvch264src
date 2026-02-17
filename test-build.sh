#!/bin/bash
# Build and test script for libuvch264src
# Run this on macOS to test Linux compilation via Docker

set -e

IMAGE_NAME="libuvch264src-builder"
CONTAINER_NAME="libuvch264src-test"

echo "🔨 Building Docker image..."
docker build -t "$IMAGE_NAME" .

echo ""
echo "🧪 Running build test..."
docker run --rm --name "$CONTAINER_NAME" "$IMAGE_NAME"

echo ""
echo "✅ All tests passed!"

