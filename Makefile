DOCKER_IMAGE = ft_irc:latest
CONTAINER_NAME = ft_irc_dev

.PHONY: all clean fclean re docker-build docker-start docker-stop
NAME = ircserv
CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98
SRCS = \
	main.cpp \
	utils.cpp \
	Server.cpp \
	Client.cpp \
	Channel.cpp \
	PassCommand.cpp \
	NickCommand.cpp \
	UserCommand.cpp
OBJDIR = obj
OBJS = $(addprefix $(OBJDIR)/, $(SRCS:.cpp=.o))

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(NAME) $(OBJS)

$(OBJDIR)/%.o: %.cpp | $(OBJDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJDIR):
	mkdir -p $(OBJDIR)

clean:
	rm -rf $(OBJS)

fclean: clean
	rm -f $(NAME)

re: fclean all

docker-start:
	@# build image only if it doesn't already exist
	@if ! docker image inspect $(DOCKER_IMAGE) >/dev/null 2>&1 ; then \
		echo "Image $(DOCKER_IMAGE) not found, building..."; \
		docker build -t $(DOCKER_IMAGE) .; \
	else \
		echo "Image $(DOCKER_IMAGE) exists, skipping build"; \
	fi
	@# ensure a persistent volume for obj so host .o files don't conflict
	@docker volume create $(CONTAINER_NAME)_obj 2>/dev/null || true
	# run an interactive shell; this command opens a bash prompt inside the container
	docker run --rm -it --name $(CONTAINER_NAME) -p 6667:6667 \
		-v "$(shell pwd)":/usr/src/app \
		-v $(CONTAINER_NAME)_obj:/usr/src/app/obj \
		-w /usr/src/app $(DOCKER_IMAGE) /bin/bash

docker-stop:
	-@docker rm -f $(CONTAINER_NAME) 2>/dev/null || true

docker-build:
	docker build -t $(DOCKER_IMAGE) .