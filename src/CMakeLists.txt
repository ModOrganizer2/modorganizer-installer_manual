cmake_minimum_required(VERSION 3.16)

add_library(installer_manual SHARED)
mo2_configure_plugin(installer_manual WARNINGS OFF)
mo2_install_target(installer_manual)
