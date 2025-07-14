NAME1    = client
NAME2    = server
SSRCS    = server.cpp
CSRCS	= client.cpp
COBJS    = $(CSRCS:.cpp=.o)
SOBJS    = $(SSRCS:.cpp=.o)
CXX     = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98

all: $(NAME1) $(NAME2)

$(NAME1) $(NAME2): $(SOBJS) $(COBJS)
	$(CXX) $(CXXFLAGS) $(COBJS) -o $(NAME1)
	$(CXX) $(CXXFLAGS) $(SOBJS) -o $(NAME2)

clean:
	rm -f $(SOBJS) $(COBJS)

fclean: clean
	rm -f $(NAME1) $(NAME2)

re: fclean all

.PHONY: all clean fclean re