#******************************************************************************#
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: carlaugu <carlaugu@student.42porto.com>    +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2026/05/22 14:48:43 by carlaugu          #+#    #+#              #
#    Updated: 2026/06/01 20:34:09 by carlaugu         ###   ########.fr        #
#                                                                              #
#******************************************************************************#

# ---------------------------------------------------------------------------- #
#                                  Variables                                   #
# ---------------------------------------------------------------------------- #

NAME     = webserv
OBJ_DIR  = obj

SRC      = src/core/main.cpp src/Parser/ConfigParser.cpp src/Parser/ConfigParserDirectives.cpp \
		src/Parser/ConfigParserUtils.cpp src/Parser/Location.cpp src/Parser/ServerConfig.cpp \
		src/core/ServerManager.cpp src/core/Client.cpp
OBJS     = $(SRC:%.cpp=$(OBJ_DIR)/%.o)
TOTAL_FILE = $(words $(SRC))

CXX      = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -g -O0

RM       = rm -rf
MSG      =

# ---------------------------------------------------------------------------- #
#                                   Commands                                   #
# ---------------------------------------------------------------------------- #

.PHONY: all save clean fclean re help

all: $(NAME)

$(NAME): $(OBJS)
	@$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)
	@echo "$(BOLD)$(GREEN)$(NAME) compiled successfully!$(RESET)"

$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(@D)
	@$(CXX) $(CXXFLAGS) -c $< -o $@
	@COUNT=$$(find $(OBJ_DIR) -name "*.o" 2>/dev/null | wc -l); \
	PERCENT=$$(echo "$$COUNT $(TOTAL_FILE)" | awk '{printf "%.0f", $$1/$$2 * 100}'); \
	printf "$(YELLOW)Compilation progress: $$PERCENT%%$(RESET)\r"

save:
	@test -n "$(MSG)" || { echo "$(BOLD)$(RED)ERROR: use make save MSG=\"message\"$(RESET)"; exit 1; }
	@git add .
	@git commit -m "$(MSG)"
	@git push

clean:
	@$(RM) $(OBJ_DIR)
	@echo "$(BOLD)$(RED)Object files removed$(RESET)"

fclean: clean
	@$(RM) $(NAME)
	@echo "$(BOLD)$(RED)Executable removed$(RESET)"

re: fclean all


help:
	@echo "$(BOLD)Usage:$(RESET)"
	@echo "  make              - Compile $(NAME)"
	@echo "  make clean        - Remove object files"
	@echo "  make fclean       - Remove object files and executable"
	@echo "  make re           - Recompile everything"
	@echo "  make save MSG=\"…\" - Commit and push with message"

# ---------------------------------------------------------------------------- #
#                                    Colors                                    #
# ---------------------------------------------------------------------------- #
RED    = \033[0;31m
GREEN  = \033[0;32m
YELLOW = \033[0;33m
BLUE   = \033[0;34m
RESET  = \033[0m
BOLD   = \033[1m