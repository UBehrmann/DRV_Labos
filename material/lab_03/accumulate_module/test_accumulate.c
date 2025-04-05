#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/ioctl.h>

#define DEVICE_PATH "/dev/accumulate"
#define ACCUMULATE_CMD_RESET 11008
#define ACCUMULATE_CMD_CHANGE_OP 1074014977
#define OP_ADD 0
#define OP_MULTIPLY 1

int main() {
	int fd;
	uint64_t value, result;

	fd = open(DEVICE_PATH, O_RDWR);
	if (fd < 0) {
		perror("Failed to open device");
		return 1;
	}

	// Reset the accumulator
	if (ioctl(fd, ACCUMULATE_CMD_RESET) < 0) {
		perror("Failed to reset accumulator");
	}else{
        printf("Accumulator reset\n");
    }

	// Write a value to the accumulator
	value = 10;
	if (write(fd, &value, sizeof(value)) != sizeof(value)) {
		perror("Failed to write value");
	}else{
        printf("Wrote value: %llu\n", value);
    }

	// Read the accumulated value
	if (read(fd, &result, sizeof(result)) != sizeof(result)) {
		perror("Failed to read value");
	}else{
        printf("Accumulated value: %llu\n", result);
    }
	

	// Change operation to multiplication
	if (ioctl(fd, ACCUMULATE_CMD_CHANGE_OP, OP_MULTIPLY) < 0) {
		perror("Failed to change operation");
	}else{
        printf("Changed operation to multiplication\n");
    }

	// Write another value
	value = 5;
	if (write(fd, &value, sizeof(value)) != sizeof(value)) {
		perror("Failed to write value");
	}else{
        printf("Wrote value: %llu\n", value);
    }

	// Read the accumulated value again
	if (read(fd, &result, sizeof(result)) != sizeof(result)) {
		perror("Failed to read value");
	}else{
        printf("Accumulated value after multiplication: %llu\n", result);
    }	

	close(fd);
	return 0;
}
