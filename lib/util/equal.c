#include "util.h"

/*
 * equal(s1, s2, n)-- compare two strings to see if they're equiv, but
 *			constrain ourselves to equal length strings.
 *
 * If both strings end at the same position and were equivalent for all
 * previous characters, then return TRUE.
 *
 * If both strings are equivalent for their first n characters,
 * then return TRUE.
 *
 * ELSE
 *
 * return FALSE.
 *
 * If n <= 0 then ignore the second condition above.
 *
 * IMPROVEMENT: sanity checks arguments to see if they're good strings.
 *		stops at the end of either string
 *		the n <= 0 thing is an added flexibility
 */
equal(str1, str2, n)
register char *str1, *str2;
int n;
{
	extern		char	chrcnv[];
	register	int	cnt;

	if (str1 == 0 || str2 == 0)
		return(str1 == str2);

	for (cnt = 1; chrcnv[*str1] == chrcnv[*str2]; str1++, str2++) {

		/*** A string ended. ***/
		if ((*str1 == 0)||(*str2 == 0)) {
			if ((*str1 == 0)&&(*str2 == 0))
				return(TRUE);
			else
				return(FALSE);
		}

		/*** Are they equiv for the first n(n>0)characters? ***/
		cnt++;
		if ((n > 0) && (cnt > n))
			return(TRUE);
	}

	return(FALSE);
}
