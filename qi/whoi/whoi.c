/*
 * this program implements a really limited version of the whois protocol
 * using qi.  No doubt people who really like whois will hate this program,
 * but it is useful due to the fact that we have a whois client for some
 * systems (macs and pcs) that are difficult to write qi clients for
 */
/*
 * the basic idea is simple; we read a line from stdin.
 * if the line is a single "?", we give a usage message and exit
 * otherwise, we run a qi, prepend a "query" to the line, send the
 * line to the qi, and pass the response back.  When we are done
 * responding, we die (kind of like salmon)
 */
#include <stdio.h>
#define LINESIZE 2048
main()
{
	char	line[LINESIZE];

	if (GetLine(line, sizeof (line), stdin))
	{
		if (*line == '?')
		{
			puts("This is a whois frontend for the CSO Nameserver.\r");
			puts("Send a name, and you'll get the ph entry back.\r");
		} else
		{
			int	whoiToqi[2];
			int	qiTowhoi[2];

			if (pipe(whoiToqi))
			{
				perror("pipe whoiToqi");
				exit(1);
			}
			if (pipe(qiTowhoi))
			{
				perror("pipe qiTowhoi");
				exit(1);
			}
			if (!fork())
			{
				/* will become qi */
				close(whoiToqi[1]);
				close(qiTowhoi[0]);

				/* set our stdin, stdout, and stderr properly */
				if (dup2(qiTowhoi[1], 1) < 0)
				{
					perror("dup2 stdout");
					exit(1);
				}
				if (dup2(qiTowhoi[1], 2) < 0)
				{
					perror("dup2 stderr");
					exit(1);
				}
				if (dup2(whoiToqi[0], 0) < 0)
				{
					perror("dup2 stdin");
					exit(1);
				}
				/* dew it */
				execl("/nameserv/bin/qi", "qi", "-d", 0);

				/* uh-oh */
				perror("qi");
				exit(1);
			} else
			{
				FILE	*toQI, *fromQI;

				/* this is really boring... */
				close(whoiToqi[0]);
				close(qiTowhoi[1]);
				if (!(toQI = fdopen(whoiToqi[1], "w")))
				{
					perror("toQI");
					exit(1);
				}
				if (!(fromQI = fdopen(qiTowhoi[0], "r")))
				{
					perror("fromQI");
					exit(1);
				}
				/* now for the good stuff... */
				fprintf(toQI, "query %s\n quit\n", line);
				fflush(toQI);

				/* print the response */
				while (fgets(line, sizeof (line), fromQI))
				{
					PutLine(line, stdout);
					if (atoi(line) >= 200)
						break;
				}

				/* trash the response from the quit */
				while (fgets(line, sizeof (line), fromQI)) ;

				/* hooray! */
				exit(0);
			}
		}
	}
	/* sob */
	exit(1);
}

/***********************************************************************
* get a line of input, including newline
***********************************************************************/
GetLine(line, size, fp)
	char	*line;
	int	size;
	FILE	*fp;
{
	int	c;
	char	*lp = line;

	while ((c = getc(fp)) != EOF)
		switch (c)
		{
		case '\r':
		case '\0':;

		case '\n':
			*lp++ = c;
			*lp = '\0';
			return (lp - line);
			break;

		default:
			if (lp - line < size - 2)
				*lp++ = c;
			break;
		}
	*lp = '\0';
	return (lp - line);
}

PutLine(line, fp)
	char	*line;
	FILE	*fp;
{
	for (; *line; line++)
		switch (*line)
		{
		case '\r':
			putc('\r', fp);
			putc('\0', fp);
			break;
		case '\n':
			putc('\r', fp);
			putc('\n', fp);
			break;
		default:
			putc(*line, fp);
			break;
		}
}
