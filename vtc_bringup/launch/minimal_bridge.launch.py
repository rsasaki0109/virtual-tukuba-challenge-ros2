from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node
from ament_index_python.packages import get_package_share_directory

from pathlib import Path


def generate_launch_description():
    package_share = Path(get_package_share_directory("vtc_bringup"))
    default_params = package_share / "config" / "vtc_bridge.params.yaml"

    transport_backend = LaunchConfiguration("transport_backend")
    simulator_host = LaunchConfiguration("simulator_host")
    vehicle_name = LaunchConfiguration("vehicle_name")

    return LaunchDescription([
        DeclareLaunchArgument("transport_backend", default_value="mock"),
        DeclareLaunchArgument("simulator_host", default_value="127.0.0.1"),
        DeclareLaunchArgument("vehicle_name", default_value=""),
        Node(
            package="vtc_bridge",
            executable="vtc_bridge_node",
            name="vtc_bridge",
            output="screen",
            parameters=[
                str(default_params),
                {
                    "transport_backend": transport_backend,
                    "simulator_host": simulator_host,
                    "vehicle_name": vehicle_name,
                },
            ],
        ),
    ])
