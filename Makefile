NAME    = Webserv
SRCS    = 	main.cpp \
			server.cpp 
OBJS    = $(SRCS:.cpp=.o)
CXX     = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98
RESET = "\033[0m"
BLACK = "\033[1m\033[37m"

all: $(NAME)
	@echo $(BLACK) webserv compiled 🌐 $(RESET)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re