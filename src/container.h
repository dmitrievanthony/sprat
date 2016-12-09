typedef struct container {
	char *image_name;
	char *init_script;
	char *id;
	long pid;
} container;

container *start_container(char *image_name, char *init_script);
