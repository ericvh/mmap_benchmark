# mmap_benchmark

This repository contains a benchmark for testing the performance of `mmap`.

## Building the Docker Container

To build the Docker container, you can use the provided GitHub Actions workflow or build it manually using Docker.

### Using GitHub Actions

The GitHub Actions workflow will automatically build, test, and push the Docker container to GitHub Container Registry whenever there is a push or pull request to the `main` branch.

### Manually Building the Docker Container

To build the Docker container manually, run the following command:

```sh
docker build -t ghcr.io/<your-username>/mmap_benchmark:latest .