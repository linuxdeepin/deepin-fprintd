#include <stdlib.h>

#include "utils.h"

void
free_strv(char **strv)
{
	if (!strv) {
		return;
	}

	int i = 0;
	for (; strv[i]; i++) {
		free(strv[i]);
	}
	free(strv);
}
