DEP=dependencies
# The 'all' target is the default one.
# It tells 'make' that the main goal is to build 'askai'.
all: askai 

# This rule builds the 'askai' executable from the source files
# and links it with the curl library.
# It runs if 'askai' doesn't exist, or if askai.c or cJSON.c have changed.
askai: askai.c $(DEP)/cJSON.c
	gcc askai.c $(DEP)/cJSON.c -o askai -lcurl

# This is a 'clean' rule to remove the compiled program.
# You can run it with the command: make clean
clean:
	rm -f askai