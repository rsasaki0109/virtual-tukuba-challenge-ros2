from setuptools import find_packages, setup

package_name = 'vtc_tools'

setup(
    name=package_name,
    version='0.0.0',
    packages=find_packages(exclude=['test']),
    data_files=[
        ('share/ament_index/resource_index/packages',
            ['resource/' + package_name]),
        ('share/' + package_name, ['package.xml']),
    ],
    install_requires=['setuptools'],
    zip_safe=True,
    maintainer='Ryohei Sasaki',
    maintainer_email='ryohei.sasaki@map4.jp',
    description='Helper scripts and reporting tools for Virtual Tsukuba Challenge ROS 2 workflows',
    license='Apache-2.0',
    extras_require={
        'test': [
            'pytest',
        ],
    },
    entry_points={
        'console_scripts': [
            'artifact_summary = vtc_tools.artifact_summary:main',
        ],
    },
)
