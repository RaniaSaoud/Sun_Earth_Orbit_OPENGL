build:
	 g++ ./src/main.cpp ./include/shader/shader.cpp ./include/model/objload.cpp -L./lib -I./include -lglfw3 -lglad -lmingw32 -lopengl32 -lgdi32 -luser32 -o out
	