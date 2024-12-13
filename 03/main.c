#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LENGTH 32768

char *
read_expression_from_file(const char* filename)
{
	FILE* file = fopen(filename, "r");
	if (!file) {
		perror("Error opening file");
		return NULL;
	}

	char* expression = malloc(MAX_LENGTH);
	if (!expression) {
		fclose(file);
		return NULL;
	}

	size_t bytes_read = fread(expression, 1, MAX_LENGTH - 1, file);
	expression[bytes_read] = '\0';

	fclose(file);
	return expression;
}

int
main(void)
{
	char *expression = read_expression_from_file("input.txt");

	// advance til mul(
	// advance til , if digits
	// advance til ) if digits

	size_t len = strlen(expression);

	// Part 1.
	int nums[1024] = {0};
	size_t nums_start = 0;
	for (size_t i = 0; i < len; ++i) {
		// found mul(
		if ((i < len && i + 1 < len && i + 2 < len && i + 3 < len) &&
			(expression[i] == 'm' && expression[i + 1] == 'u' && expression[i + 2] == 'l' && expression[i + 3] == '(')) {
			// parse the first digit till ','
			size_t start = 4;
			size_t digit_start = 0;
			char digit_str[16] = {0};
			int first_digit = 0;
			while (isdigit(expression[i + start]) && (expression[i + start] != ',' || i + start < len)) {
				digit_str[digit_start] = expression[i + start];
				++start;
				++digit_start;
			}
			digit_str[digit_start] = '\0';
			first_digit = atoi(digit_str);

			digit_start = 0;
			memset(digit_str, 0, strlen(digit_str));

			// parse the second digit till ')'
			int second_digit = 0;
			++start;
			while (isdigit(expression[i + start]) && (expression[i + start] != ')' || i + start < len)) {
				digit_str[digit_start] = expression[i + start];
				++start;
				++digit_start;
			}
			if (expression[i + start] != ')') continue;
			digit_str[digit_start] = '\0';
			second_digit = atoi(digit_str);

			nums[nums_start] = first_digit * second_digit;

			++nums_start;
		}
	}

	int sum = 0;
	for (size_t i = 0; i < nums_start; ++i) {
		sum += nums[i];
	}

	printf("sum 1 =\n\t%d\n", sum);

	// Part 2.
	nums_start = 0;
	bool cancel = false;
	bool consecutive_cancel = false;
	for (size_t i = 0; i < len; ++i) {

		// found do()
		if ((i < len && i + 1 < len && i + 2 < len && i + 3 < len) &&
			(expression[i] == 'd' && expression[i + 1] == 'o' && expression[i + 2] == '(' && expression[i + 3] == ')')) {
			cancel &= 0;
			consecutive_cancel = true;
		}

		// found don't()
		else if ((i < len && i + 1 < len && i + 2 < len && i + 3 < len && i + 4 < len && i + 5 < len && i + 6 < len) &&
			(expression[i] == 'd' && expression[i + 1] == 'o' && expression[i + 2] == 'n' && expression[i + 3] == '\'' && expression[i + 4] == 't' && expression[i + 5] == '(' && expression[i + 6] == ')')) {
			cancel |= 1;
			consecutive_cancel = true;
		}

		// the next is a mul expression
		else {
			consecutive_cancel = false;
		}

		// found mul(
		if (!cancel && !consecutive_cancel && (i < len && i + 1 < len && i + 2 < len && i + 3 < len) &&
			(expression[i] == 'm' && expression[i + 1] == 'u' && expression[i + 2] == 'l' && expression[i + 3] == '(')) {
			// parse the first digit till ','
			size_t start = 4;
			size_t digit_start = 0;
			char digit_str[16] = {0};
			int first_digit = 0;
			while (isdigit(expression[i + start]) && (expression[i + start] != ',' || i + start < len)) {
				digit_str[digit_start] = expression[i + start];
				++start;
				++digit_start;
			}
			digit_str[digit_start] = '\0';
			first_digit = atoi(digit_str);

			digit_start = 0;
			memset(digit_str, 0, strlen(digit_str));

			// parse the second digit till ')'
			int second_digit = 0;
			++start;
			while (isdigit(expression[i + start]) && (expression[i + start] != ')' || i + start < len)) {
				digit_str[digit_start] = expression[i + start];
				++start;
				++digit_start;
			}
			if (expression[i + start] != ')') continue;
			digit_str[digit_start] = '\0';
			second_digit = atoi(digit_str);

			nums[nums_start] = first_digit * second_digit;

			++nums_start;
		}
	}

	sum = 0;
	for (size_t i = 0; i < nums_start; ++i) {
		sum += nums[i];
	}

	printf("sum 2 =\n\t%d\n", sum);

	return 0;
}
