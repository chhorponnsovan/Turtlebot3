#!/bin/bash
set -e

# Source ROS setup
source /opt/ros/humble/setup.bash

# Execute the command passed to the container
exec "$@"