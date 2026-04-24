# ================================
# STM32 GDB 初始化脚本 (.gdbinit)
# 适用于: OpenOCD + arm-none-eabi-gdb / cgdb
# ================================

# NOTE: Mac 下需要在 Shell 中执行: echo "set auto-load safe-path /" >> ~/Library/Preferences/gdb/gdbinit
# 否则不能自动加载 .gdbinit 文件

# 关闭分页输出
# 默认情况下，如果输出很多内容（如 disassemble、info registers）
# GDB 会暂停并显示 "--Type <RET> for more--"
# 关闭后所有内容会一次性输出
set pagination off


# 关闭确认提示
# 默认很多命令需要 y/n 确认，比如重新加载程序
# 关闭后脚本执行不会被中断
set confirm off


# 打印复杂结构体时自动格式化
# 对调试结构体、数组、链表等非常有用
set print pretty on


# --------------------------------
# 连接 OpenOCD GDB Server
# --------------------------------
# OpenOCD 默认启动一个 GDB server
# 地址通常是 localhost:3333
#
# 架构：
# GDB / cgdb
#      │
#      │ TCP
#      ▼
# OpenOCD (gdb server)
#      │
#      │ SWD / JTAG
#      ▼
# STM32 MCU
target remote localhost:3333


# --------------------------------
# 复位 MCU 并暂停 CPU
# --------------------------------
# monitor 命令表示把命令发送给 OpenOCD
#
# reset halt 的作用：
# 1. 复位 MCU
# 2. 在 reset 后立即暂停 CPU
#
# CPU 会停在 Reset_Handler
monitor reset halt


# --------------------------------
# 开启 semihosting
# --------------------------------
# semihosting 是 ARM 提供的一种调试功能
# MCU 可以通过调试器向主机输出信息
#
# 常见用途：
# printf 输出到 GDB 控制台
# 文件操作
#
# 如果不需要可以删除
monitor arm semihosting enable


# --------------------------------
# 下载程序到 MCU Flash
# --------------------------------
# load 会把当前 ELF 文件中的
# .text / .data 等 section
# 写入 MCU Flash
#
# 相当于烧录程序
load


# --------------------------------
# 在 main 函数设置断点
# --------------------------------
# STM32 启动流程:
#
# Reset_Handler
#    ↓
# SystemInit
#    ↓
# __libc_init_array
#    ↓
# main()
#
# 这里让程序在 main() 停住
break main


# --------------------------------
# 自定义命令: reset
# --------------------------------
# 在 GDB 里输入:
#
# reset
#
# 就等价于:
#
# monitor reset halt
#
define reset
    monitor reset halt
end


# --------------------------------
# 自定义命令: flash
# --------------------------------
# 在 GDB 里输入:
#
# flash
#
# 就会重新下载程序
#
define flash
    load
end

# --------------------------------
# 自定义命令: reboot
# --------------------------------
# 完全重启调试：
# 1. 复位芯片并暂停:
#       - monitor: 告诉 gdb 不要自己处理这条命令, 转发给 OpenOCD 执行
#       - halt: 复位后暂停 cpu 执行, 否则芯片复位后会直接运行
# 2. 重新烧录固件
# 3. 将 PC 指向 Reset_Handler
#
# 使用方法：reboot
define reboot
    # 复位并停止
    monitor reset halt
    # 重新烧录程序
    load
    # 设置程序计数器
    set $pc = Reset_Handler
    echo \n--- restart success, start at Reset_Handler ---\n
end

echo "================ .gdbinit LOADED ================\n"

