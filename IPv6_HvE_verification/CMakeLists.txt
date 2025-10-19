# CMakeList.txt: IPv6_HvE_verification 的 CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#

# 设置最低 CMake 版本和项目名称 (建议在文件顶部添加 project 命令)
cmake_minimum_required(VERSION 3.10)
project(IPv6_HvE_verification CXX)

# 将源代码添加到此项目的可执行文件。
add_executable (IPv6_HvE_verification
    "IPv6_HvE_verification.cpp"
    "IPv6_HvE_verification.h"
    "utils.cpp"
    "utils.h"
    "2D_chessboard.cpp"
    "Pdata.cpp"
    "2D_to_1D.cpp"
    "DataFel.cpp"
    "Xadd.cpp"
    "Column_encoding.cpp"
    "IDonin.cpp"
    "IDrrg.cpp"
    "Datadismantle.cpp"
)

# 设置 C++ 标准
if (CMAKE_VERSION VERSION_GREATER 3.12)
set_property(TARGET IPv6_HvE_verification PROPERTY CXX_STANDARD 20)
endif()


# ====================================================================
# OpenSSL 库查找 (Vcpkg优先, 本地回退)
# ====================================================================

# 尝试使用现代/vcpkg方式查找 OpenSSL (寻找 OpenSSL::* targets)
# QUIETLY 避免在未找到时立即报错
find_package(OpenSSL QUIET)

if(OpenSSL_FOUND)
    # -----------------------------------------------------------------
    # VCPKG/现代 CMake 方式
    # -----------------------------------------------------------------
    message(STATUS "OpenSSL 库已找到 (使用 Vcpkg 或现代 find_package 目标)")

    # 链接 OpenSSL 目标
    target_link_libraries(IPv6_HvE_verification PRIVATE
        OpenSSL::SSL
        OpenSSL::Crypto
    )

elseif(NOT OpenSSL_FOUND)
    # -----------------------------------------------------------------
    # 传统/本地路径方式 (回退机制)
    # -----------------------------------------------------------------

    message(STATUS "OpenSSL 库未找到现代目标，尝试使用传统方式查找本地库...")

    # 如果 vcpkg 没找到，尝试指定本地路径（根据您的原始代码）
    # 用户可以在这里修改路径或在命令行传入 -DOpenSSL_ROOT_DIR=...
    if(NOT OpenSSL_ROOT_DIR)
        set(OpenSSL_ROOT_DIR "C:/Program Files/OpenSSL-Win64" CACHE PATH "Path to the local OpenSSL installation")
    endif()

    # 使用传统方式再次查找 OpenSSL
    # 此时 find_package 可能会使用我们设置的 OpenSSL_ROOT_DIR
    find_package(OpenSSL QUIET)

    if(OPENSSL_FOUND)
        message(STATUS "OpenSSL 库已找到 (使用本地路径回退)")

        # 将 OpenSSL 的头文件目录添加到目标
        target_include_directories(IPv6_HvE_verification PRIVATE ${OPENSSL_INCLUDE_DIR})

        # 将 OpenSSL 库链接到目标
        target_link_libraries(IPv6_HvE_verification PRIVATE ${OPENSSL_LIBRARIES})

    else()
        # -----------------------------------------------------------------
        # 最终警告
        # -----------------------------------------------------------------
        message(WARNING "OpenSSL 库未找到。项目可能无法编译或运行时缺少功能。")
    endif()

endif()

# -----------------------------------------------------------------
# TODO: 如有需要，请添加测试并安装目标。
# -----------------------------------------------------------------