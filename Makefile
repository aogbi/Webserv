NAME    = Webserv
SRCDIR  = srcs
HEADERDIR = headers
SRCS    = 	$(SRCDIR)/main.cpp \
			$(SRCDIR)/server.cpp \
			$(SRCDIR)/request.cpp \
			$(SRCDIR)/response.cpp \
			$(SRCDIR)/signal_handler.cpp \
			$(SRCDIR)/utils.cpp \
			$(SRCDIR)/config.cpp \
			$(SRCDIR)/http_handler.cpp \
			$(SRCDIR)/cgi_handler.cpp \
			$(SRCDIR)/connection_manager.cpp 
OBJS    = $(SRCS:.cpp=.o)
CXX     = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -fPIE -I$(SRCDIR) -I$(HEADERDIR)
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