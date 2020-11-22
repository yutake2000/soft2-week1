#include <stdio.h>

int number_len(int n) {
	int len = 0;
	while(n > 0) {
		len++;
		n /= 10;
	}

	return len;
}

int isWhitespace(char c) {
	return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

int main() {
	
	FILE *fp;

	fp = fopen("Pulsar.rle", "r");
	if (fp == NULL) {
		return 1;
	}

	int height = 40;
	int width = 70;
	int cell[height][width];
	for(int y = 0 ; y < height ; y++){
		for(int x = 0 ; x < width ; x++){
			cell[y][x] = 0;
		}
	}

	char buffer[100];
	int H = 0, W = 0;
	char rule_B[100];
	char rule_S[100];
	int y = 0, x = 0;
	int offsetY = 0, offsetX = 0;
	while(fgets(buffer, 99, fp) != NULL) {

		if (buffer[0] == '#') {
			if (buffer[1] == 'P' || buffer[1] == 'R') {
				sscanf(buffer+2, "%d%d", &offsetX, &offsetY);
				printf("offset: %d %d\n", offsetX, offsetY);
				y = offsetY;
				x = offsetX;
			}
			continue;
		}

		if (buffer[0] == 'x') {
			int result = sscanf(buffer, "x = %d, y = %d, rule = B%[^/]/S%s", &W, &H, rule_B, rule_S);
			printf("%d\n", result);
		} else {
			int offset = 0;
			while(1) {
				int len = 0;
				char c;

				while(isWhitespace(buffer[offset])) offset++;

				int result = sscanf(buffer+offset, "%d", &len);
				printf("result: %d, len: %d\n", result, len);
				offset += number_len(len);

				if (len == 0) len++;

				result = sscanf(buffer+offset, "%c", &c);
				printf("result: %d, c: %c\n", result, c);
				offset++;

				if (result <= 0 || c == '!') {
					break;
				} else if (c == '$') {
					y += len;
					x = offsetX;
				} else if (c == 'b') {
					x += len;
				} else if (c == 'o') {
					for (int i=0; i<len; i++) {
						cell[y][x] = 1;
						x++;
					}
				} else if (c == '\n' || c == '\r') {
					printf("CRLF");
					break;
				} else {
					fprintf(stderr,"Invalid syntax\n");
					break;
				}
			}
		}

	}

	fclose(fp);

	for (int y=0; y<height; y++) {
		for (int x=0; x<width; x++) {
			printf("%c", (cell[y][x] ? '#' : '.'));
		}
		printf("\n");
	}

	printf("x = %d, y = %d, rule = %s/%s\n", W, H, rule_B, rule_S);

	return 0;
}