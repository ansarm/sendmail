// sendmail.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <winsock.h>
#include <stdio.h>
#include <io.h>
#include <windows.h>
#include <time.h>


struct sockaddr_in A;
WSADATA W;
SOCKET S;
char aa[100];
int i;
struct hostent *H;
char R[10000];
char base64alphabet[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";


int nullstring(char *s)
{
	int loop;
	for (loop = 0; loop <= 9999; loop++)
		s[loop]= '\0';
	return 0;

}

int base64(char * in3bytes, char* out4bytes, int padbitcount)
{
	out4bytes[0]=base64alphabet[((in3bytes[0] & 0xFC) >> 2)];
	out4bytes[1]= base64alphabet[(((in3bytes[0] & 0x3) <<4 ) | ((in3bytes[1] & 0xF0) >>4))];
	if (padbitcount >0)
		out4bytes[2]='=';
	else
		out4bytes[2]=  base64alphabet[(((in3bytes[1] & 0xF ) << 2) | ((in3bytes[2] & 0xC0) >>6))];
	if (padbitcount >1)
		out4bytes[3]='=';
	else
		out4bytes[3]= base64alphabet[(in3bytes[2] & 0x3F)];
	return 0;
}


int main(int argc, char* argv[])
{

char server[64], sender[64], recipient[64], subject[64], 
	message[1024], localhost[64], attachment[64], spoofedrelay[64];


int error;
int loop;
int got_sender =0;
int got_server =0;
int got_recipient =0;
int got_subject =0;
int got_message =0;
int got_sourcehost=0;
int got_attachment=0;
int got_verbose=0;
int got_spoofedrelay=0;
int got_binary=0;


/* for base64 conversion */
FILE * file_attachment;
char in3bytes[3], out4bytes[4];
unsigned int c;
int charcount=0;
int padbitcount=0;
int gotlinefeed =0;
	

time_t localtime;
char thetime[64];

	for (loop = 1; loop < argc; loop++)
	{
		if (strcmp (argv[loop], "-r")== 0)
		{
			if (strlen(argv[loop+1]) > 64)
			{
				printf ("message to large\n");
				return 0;
			}
			strcpy(recipient, argv[loop+1]);
			got_recipient =1;
		
		}
	
		if (stricmp (argv[loop], "-f")== 0)
		{
			if (strlen(argv[loop+1]) > 64)
			{
				printf ("sender address to large\n");
				return 0;
			}
			strcpy(sender, argv[loop+1]);
			got_sender=1;
		
		}
	

		if (stricmp (argv[loop], "-s")== 0)
		{
			if (strlen(argv[loop+1]) > 64)
			{
				printf ("server name to large\n");
				return 0;
			}
			strcpy(server, argv[loop+1]);
			got_server=1;
		
		}
	
		if (stricmp (argv[loop], "-t")== 0)
		{
			if (strlen(argv[loop+1]) > 64)
			{
				printf ("topic to large\n");
				return 0;
			}
			strcpy(subject, argv[loop+1]);
			got_subject=1;
		
		}
	
		if (stricmp (argv[loop], "-m")== 0)
		{
			if (strlen(argv[loop+1]) > 1024)
			{
				printf ("message to large\n");
				return 0;
			}
			strcpy(message, argv[loop+1]);
			got_message=1;
		
		}
		   
   
		if (stricmp (argv[loop], "-h")== 0)
		{
			if (strlen(argv[loop+1]) > 64)
			{
				printf ("sourcehost to large\n");
				return 0;
			}
			strcpy(localhost, argv[loop+1]);
			got_sourcehost=1;
		}
		
		if (stricmp (argv[loop], "-sr")== 0)
		{
			if (strlen(argv[loop+1]) > 64)
			{
				printf ("spoofed relay host too large\n");
				return 0;
			}
			strcpy(spoofedrelay, argv[loop+1]);
			got_spoofedrelay=1;
		}
		
		if (stricmp (argv[loop], "-a")== 0)
		{
			if (strlen(argv[loop+1]) > 1024)
			{
				printf ("attachment pathname to large\n");
				return 0;
			}
			strcpy(attachment, argv[loop+1]);
			got_attachment=1;
		}
		
		if (stricmp (argv[loop], "-b")== 0)
			got_binary=1;
		

		if (stricmp (argv[loop], "-v")== 0)
		{
			got_verbose=1;
			printf("Sending Mail ....\nFrom:%s\nTo:%s\nUsing Server:%s\nSubject:%s\nMessage:%s\n\n\n",
				sender, recipient, server, subject, message );
		}
	}

if ((got_sender + got_server + got_recipient + got_subject + got_message ) != 5)
	{
		if (got_sender==0)
			printf ("must include sender\n");
		if (got_recipient==0)
			printf ("must include recipient\n");
		if (got_server == 0)
			printf ("must include server\n");
		if (got_subject == 0)
			printf ("must include subject\n");
		if (got_message == 0)
			printf ("must include message\n");
		printf ("usage:\n sendmail\n");
		printf ("   -r recipient	\t: Recipient's Address\n");
		printf ("   -f sender	\t: Sender's Address\n");
		printf ("   -s server	\t: Server's IP Address or hostname\n");
		printf ("   -t subject	\t: Mail Subject (use double quotes for multiple words)\n");
		printf ("   -m message	\t: Mail Message (use double quotes for multiple words)\n");
		printf ("  [-h sourcehost] \t: Optional source host\n");
		printf ("  [-a attachment] \t: Attachment file (text only and without full path)\n");
		printf ("  [-b ] \t\t: Binary attachment file (text is default)\n");
		printf ("  [-sr spoofedrelay] \t: Optional fake relay servers\n");
		printf ("  [-v]  \t\t: Optional verbose debugging mode\n");		
		printf("Version 2.0 by Ansar Mohammed\n");
		return 0;
	}

  error= WSAStartup (0x101, &W);
  if (error != 0)
  {
	  printf("Unable to initalize Winsock\n");
	  return 0;
  }

if (got_sourcehost==0)
		gethostname(localhost, 63);		

  S = socket(AF_INET, SOCK_STREAM,0);
  A.sin_family=AF_INET;
  A.sin_port = htons(25);
  H=gethostbyname(server);
  if (H == NULL)
  {
	  printf("Unable to resolve hostname\n");
	  return 0;
  }


  A.sin_addr.s_addr=*((unsigned long *) H->h_addr);
  i=connect(S,(struct sockaddr *) &A, sizeof(A));
  nullstring(R);
  i=recv(S,R,10000,0);
  if (got_verbose==1)
	  printf("%s",R);
  
  /* setup hostname */b
  
  strcpy(R, "HELO ");
  strcat(R, localhost);
  strcat(R, "\r\n");
  i=send(S,R,strlen(R),0);
  nullstring(R);;
  i=recv(S,R,10000,0);
  if (got_verbose==1)
	  printf("%s",R);
  

  /*setup sender */
  strcpy(R, "MAIL FROM:<");
  strcat(R, sender);
  strcat(R, ">\r\n");
  i=send(S,R,strlen(R),0);
  nullstring(R);
  i=recv(S,R,10000,0);
  if (got_verbose==1)
	  printf("%s",R);

  /*setup recipient */
  strcpy(R,"RCPT  TO:<");
  strcat(R, recipient);
  strcat(R, ">\r\n");
  i=send(S,R,strlen(R),0);
  nullstring(R);
  i=recv(S,R,10000,0);
  if (got_verbose==1)
	  printf("%s",R);

  /*setup data */
  strcpy(R,"DATA\r\n");
  i=send(S,R,strlen(R),0);
  nullstring(R);
  i=recv(S,R,10000,0);
  if (got_verbose==1)
	  printf("%s",R);

  if (got_spoofedrelay==1)
  {
	/*setup spoofed relay host */
	strcpy(R, "Received: from ");
	strcat(R, spoofedrelay);
	strcat(R, " by ");
	strcat(R, localhost);
	strcat(R, " on ");

	/* setup time */
	time (&localtime);
	strcpy(thetime,ctime(&localtime));
	/*strip \n from the time */
	thetime[(strlen(thetime)-1)]='\0';
	strcat(R, thetime);
	strcat(R, "\r\n");
	i=send(S,R,strlen(R),0);
  }
  
  /*setup To */
  strcpy(R, "To: ");
  strcat(R, recipient);
  strcat(R, "\r\n");

  i=send(S,R,strlen(R),0);
  
  /*setup From */
  strcpy(R, "From: ");
  strcat(R, sender);
  strcat(R, "\r\n");
  i=send(S,R,strlen(R),0);
  
  
  /*setup subject */
  strcpy(R, "Subject: ");
  strcat(R, subject);
  strcat(R, "\r\n");
  i=send(S,R,strlen(R),0);
  

  /*setup X-Mailer */
  strcpy(R, "X-Mailer: sendmail version 2.0 by Ansar Mohammed\r\n");
  i=send(S,R,strlen(R),0);
  
  
  /*setup attachment */
	if (got_attachment==1)
	{		
		/*main mime headers */
		strcpy(R, "MIME-Version: 1.0\r\n");
		i=send(S,R,strlen(R),0);
				
		strcpy(R, "Content-Type: multipart/mixed;\r\n\tboundary=\"");
		strcat(R,"----");
		strcat(R,"\"\r\n");
		i=send(S,R,strlen(R),0);
		
		/*send boundary */
		strcpy(R,"------\r\n");
		i=send(S,R,strlen(R),0);

		strcpy(R, "Content-Type: text/plain;\r\n\tcharset=\"");
		strcat(R,"iso-8859-1\"\r\n");
		i=send(S,R,strlen(R),0);
		
		strcpy(R, "Content-Transfer-Encoding: quoted-printable\r\n\r\n");
		i=send(S,R,strlen(R),0);

		strcpy(R,message);
		i=send(S,R,strlen(R),0);
		strcpy(R,"\r\n");
		i=send(S,R,strlen(R),0);

		/*send boundary */
		strcpy(R,"------\r\n");
		i=send(S,R,strlen(R),0);


		/* RFC says this is most generic content type.. */
		strcpy(R, "Content-Type: application/octet-stream;\r\n\tname=\"");
		strcat(R,attachment);
		strcat(R,"\"\r\n");
		i=send(S,R,strlen(R),0);
		
		strcpy(R, "Content-Transfer-Encoding: base64\r\n");
		i=send(S,R,strlen(R),0);

		strcpy(R,"Content-Disposition: attachment;\r\n\tfilename=\"");
		strcat(R, attachment);
		strcat(R,"\"\r\n\r\n");
		i=send(S,R,strlen(R),0);
		
			
		/* open file and read characters*/
		file_attachment =fopen(attachment,"rb");
		c=fgetc(file_attachment);
		while (c != EOF)
		{
			
			for (loop=0; loop <3; loop++)
			{
				if (gotlinefeed == 1)
				{
					gotlinefeed=0;
					in3bytes[loop]='\n';
					c=fgetc(file_attachment);
				}
				else
				{
					if (c == EOF)
					{
						in3bytes[loop]= '\0';
						padbitcount++;
						printf("%d,",padbitcount);
					}
					else
					{
						if ((c == '\n') &&( got_binary == 0) )
						{
							printf("text attachment");
							in3bytes[loop] = '\r';
								gotlinefeed =1;
						}
						else
						{
							in3bytes[loop] = c;
							c=fgetc(file_attachment);
						}
					}
				}
			}
			

			/* convert to base 64 */
			base64(in3bytes,out4bytes,padbitcount);
			if (charcount<74)
			{
				charcount += 4;
				i=send(S, out4bytes,4,0);
			}
			else
			{
				charcount =4;
				i=send(S,"\r\n",2,0);
				i=send(S, out4bytes,4,0);
			}
		
		}
	





	strcpy(R,"\r\n\r\n--------\r\n");
	i=send(S,R,strlen(R),0);	
		
	fclose(file_attachment);
	
	}
	  /*send message */
	else
	{
		strcpy(R,message);
		i=send(S,R,strlen(R),0);
		strcpy(R,"\r\n");
		i=send(S,R,strlen(R),0);
	}



  
  /*send '.' */
  strcpy(R,".\r\n");
  i=send(S,R,strlen(R),0);
  nullstring(R);
  i=recv(S,R,10000,0);
  if (got_verbose==1)
	  printf("%s",R);


  /*send QUIT */
  strcpy(R,"QUIT\r\n");
  i=send(S,R,strlen(R),0);
  nullstring(R);
  i=recv(S,R,10000,0);
   if (got_verbose==1)
	  printf("%s",R);

  /*cleanup sockets */
  WSACleanup();

	return 0;
}



