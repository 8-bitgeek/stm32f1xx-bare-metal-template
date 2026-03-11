# ============================================================================
# colors.mk - ANSI Color Codes for Makefile
# 使用方法: 在 Makefile 中引入:
#   include colors.mk
#   @echo "$(GREEN)Hello$(NC)"
# ============================================================================

# 常规颜色
C_BLACK   = \033[0;30m
C_RED     = \033[0;31m
C_GREEN   = \033[0;32m
C_YELLOW  = \033[0;33m
C_BLUE    = \033[0;34m
C_MAGENTA = \033[0;35m
C_CYAN    = \033[0;36m
C_WHITE   = \033[0;37m

# 粗体颜色
C_BOLD_BLACK   = \033[1;30m
C_BOLD_RED     = \033[1;31m
C_BOLD_GREEN   = \033[1;32m
C_BOLD_YELLOW  = \033[1;33m
C_BOLD_BLUE    = \033[1;34m
C_BOLD_MAGENTA = \033[1;35m
C_BOLD_CYAN    = \033[1;36m
C_BOLD_WHITE   = \033[1;37m

# 背景颜色
C_BG_BLACK   = \033[40m
C_BG_RED     = \033[41m
C_BG_GREEN   = \033[42m
C_BG_YELLOW  = \033[43m
C_BG_BLUE    = \033[44m
C_BG_MAGENTA = \033[45m
C_BG_CYAN    = \033[46m
C_BG_WHITE   = \033[47m

# 特殊效果
C_BOLD        = \033[1m
C_DIM         = \033[2m
C_UNDERLINE   = \033[4m
C_BLINK       = \033[5m
C_REVERSE     = \033[7m
C_HIDDEN      = \033[8m

# 重置所有属性
C_NC = \033[0m  # No Color

# 一些常用的组合（可选）
C_INFO  = $(GREEN)
C_WARN  = $(YELLOW)
C_ERROR = $(RED)
C_DEBUG = $(CYAN)

# 辅助函数：带颜色的输出
# 用法: $(call c_print,C_GREEN,Hello World)
c_print = echo -e "$($(1))$(2)$(NC)"
