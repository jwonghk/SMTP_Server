#include "netbuffer.h"
#include "mailuser.h"
#include "server.h"
#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <ctype.h>
#include <regex.h> 

#define MAX_LINE_LENGTH 1024

typedef enum state {
    Undefined,
    EcloCalled,
    MailCalled,
    RecptCalled,
    DataCalled,
    QuitCalled
    // TODO: Add additional states as necessary
} State;

typedef struct smtp_state {
    int fd;
    net_buffer_t nb;
    char recvbuf[MAX_LINE_LENGTH + 1];
    char *words[MAX_LINE_LENGTH];
    int nwords;
    State state;
    struct utsname my_uname;
    user_list_t mail_list_receipts;
    // TODO: Add additional fields as necessary
} smtp_state;
    
static void handle_client(int fd);

int main(int argc, char *argv[]) {
  
    if (argc != 2) {
	fprintf(stderr, "Invalid arguments. Expected: %s <port>\n", argv[0]);
	return 1;
    }
  
    run_server(argv[1], handle_client);
  
    return 0;
}

// syntax_error returns
//   -1 if the server should exit
//    1  otherwise
int syntax_error(smtp_state *ms) {
    if (send_formatted(ms->fd, "501 %s\r\n", "Syntax error in parameters or arguments") <= 0) return -1;
    return 1;
}

// checkstate returns
//   -1 if the server should exit
//    0 if the server is in the appropriate state
//    1 if the server is not in the appropriate state
int checkstate(smtp_state *ms, State s) {
    if (ms->state != s) {
	if (send_formatted(ms->fd, "503 %s\r\n", "Bad sequence of commands") <= 0) return -1;
	return 1;
    }
    return 0;
}

// All the functions that implement a single command return
//   -1 if the server should exit
//    0 if the command was successful
//    1 if the command was unsuccessful

int do_quit(smtp_state *ms) {
    dlog("Executing quit\n");
    // TODO: Implement this function
    send_formatted(ms->fd, "221 %s Service closing transmission channel\r\n", ms->my_uname.nodename);
    return -1;
}

int do_helo(smtp_state *ms) {
    dlog("Executing helo\n");
    // TODO: Implement this function
    uname(&(ms->my_uname));
    send_formatted(ms->fd, "250 %s Service ready\r\n", ms->my_uname.nodename);
    ms->state = EcloCalled;
    return 0;
}

// reset the state to original
// user_list_destroy
int do_rset(smtp_state *ms) {
    dlog("Executing rset\n");
    // TODO: Implement this function
    send_formatted(ms->fd, "250 OK\r\n");
    ms->state = EcloCalled;
    user_list_destroy(ms->mail_list_receipts);
    ms->mail_list_receipts = NULL;
    
    return 0;
}

int do_mail(smtp_state *ms) {
    dlog("Executing mail\n");
    
    // TODO: Implement this function
    if(ms->nwords == 1 || ms->nwords > 2) {
        return syntax_error(ms);
    } else {
       
        char* x4 = strstr(ms->words[1], "FROM:<");
        char* x7 = strchr(ms->words[1], '>');
        if(x4 == NULL || x7 == NULL) {
            return syntax_error(ms);
        } else {
            int i = checkstate(ms, EcloCalled);
            if(i == 0) {
            send_formatted(ms->fd, "250 %s \r\n", "Requested mail action ok, completed");
            ms->state = MailCalled;
            return i;
            }
            return i; 
        }
    }
}     

int do_rcpt(smtp_state *ms) {
    dlog("Executing rcpt\n");
    // TODO: Implement this function
    
    if(ms->nwords == 1 || ms->nwords > 2) {
        return syntax_error(ms);
    } else {

        // check if the command includes the string TO:<> such as TO:<abc>, TO:<bd@hotmail.com>, etc
        int i;
        char* x4 = strstr(ms->words[1], "TO:<");
        char* x7 = strchr(ms->words[1], '>');
        if(x4 == NULL || x7 == NULL) {
            return syntax_error(ms);
        } else {
            //printf("true or false\n");
            printf("Is list currently NULL , 1 = TRUE, 0 = F: %d", ms->mail_list_receipts == NULL);
            
            
            // if no one has been added to the user list yet for sending emails to
            if(ms->mail_list_receipts == NULL) {
                printf("this 1 \n");
                i = checkstate(ms, MailCalled);
                

            //if someone has been included already in an email list
            } else if (user_list_len(ms->mail_list_receipts) > 0) {
                printf("this 2 \n");
                i = checkstate(ms, RecptCalled);
            }

            // if we are in a state that allowed to call RECPT
            if(i == 0) {
                printf("%s \n", "Allowed to call RCPT");
                char *ret;
                char *end;
                //char stripFrontBack;
                const char c = '<';
                ret = strchr(ms->words[1], c);
                const char *stripFrontBack = ret;
                stripFrontBack++;
                //stripFrontBack[strlen(stripFrontBack) -1] = '\0';
                end = strchr(stripFrontBack, '>');
                const char* final = strndup(stripFrontBack, end - stripFrontBack);
                printf("Person name checked in do_RCPT : %s \n", final);
                if(is_valid_user(final, NULL)) {
                    printf("this person: %s added!\n", final);
                    user_list_add(&ms->mail_list_receipts, final);
                    ms->state = RecptCalled;
                     send_formatted(ms->fd, "250 Requested mail action ok, completed %s\r\n", ms->my_uname.nodename);
                } else {
                    send_formatted(ms->fd, "550 No such user%s\r\n", ms->my_uname.nodename);
                }

            return i;
            }
            return i; 
        }
    }
    return 0;
}     

int do_data(smtp_state *ms) {
    dlog("Executing data\n");
    // TODO: Implement this function
    // if mkstemp is good, read something buffer to the file
    // 
    if(user_list_len(ms->mail_list_receipts) == 0){
        //send_formatted(ms->fd, "  %s\r\n", ms->my_uname);
        if (send_formatted(ms->fd, "503 %s\r\n", "Bad sequence of commands") <= 0) return -1;
	    return 1;
    }

    //char bufferForData[1024+1];
    if(checkstate(ms, RecptCalled) == 0){
        send_formatted(ms->fd, "354 Waiting for data, finish with <CR><LF>.<CR><LF> %s\r\n", ms->my_uname.nodename);
    }


    char bufferForFileName[20];
    strncpy(bufferForFileName,"TempFileMesg-XXXXXX",20);
    int FdforMkstemp = mkstemp(bufferForFileName);
    if(FdforMkstemp == -1) {
        printf("There is error in creating temp file using mkstemp function!\n");
    } 

    
    while(nb_read_line(ms->nb, ms->recvbuf ) != -1 ) {
        printf("PRINTINT data in the buffer: %s\n" , ms->recvbuf);
        if(ms->recvbuf[0] == '.' && ms->recvbuf[1] == '\r') {
            send_formatted(ms->fd, "250 ok \r\n");
            break;
        } else if(ms->recvbuf[0] == '.' && ms->recvbuf[1] == '.') {
            char *temp;
            memmove(temp, (ms->recvbuf)+1, strlen(ms->recvbuf)-1);
            write(FdforMkstemp, temp, strlen(ms->recvbuf)-1);
        } else {
            write(FdforMkstemp, ms->recvbuf, strlen(ms->recvbuf));
        }
    }
    save_user_mail(bufferForFileName, ms->mail_list_receipts);
    unlink(bufferForFileName);
    

    printf("finished entering from Client!");
  
    
    return 0;
}     
      
int do_noop(smtp_state *ms) {
    dlog("Executing noop\n");
    // TODO: Implement this function
    send_formatted(ms->fd, "250 OK%s\r\n", ms->my_uname.nodename);
    return 0;
}

int do_vrfy(smtp_state *ms) {
    dlog("Executing vrfy\n");
    // TODO: Implement this function
    // no password needed? so  NULL in 2nd arg
    // what userlist to check against? is it the users.txt
 
    // case 1: syntax error
    char *ret;
    const char c = '@';
    
    if(ms->nwords == 1) {
        return syntax_error(ms);    
    } else {
        //printf("die here? 1\n");
        ret = strchr(ms->words[1], c);
        //printf("die here? 2\n");

        if( ret == NULL || 
        ms->nwords > 3 || ms->nwords < 2) {
            return syntax_error(ms);
        } else {

    
            // case 2: no password provided
        if(ms->words[2] == NULL) {
            if(is_valid_user(ms->words[1], NULL)) {

                send_formatted(ms->fd, "250 %s\r\n", ms->words[1]);
            } else {
                send_formatted(ms->fd, "550 %s\r\n", "users not found!");
            }

            // case 3: pasword provided
        } else if(!(ms->words[2] == NULL) && (ms->words[3] == NULL)){
            if(is_valid_user(ms->words[1], ms->words[2])) {
                send_formatted(ms->fd, "250 %s\r\n", ms->words[1]);
            } else {
                send_formatted(ms->fd, "550 %s\r\n", "password not correct!");
                }
            }
        }
    }  
    return 0;
}

void handle_client(int fd) {
  
    size_t len;
    smtp_state mstate, *ms = &mstate;
  
    ms->fd = fd;
    ms->nb = nb_create(fd, MAX_LINE_LENGTH);
    ms->state = Undefined;
    ms->mail_list_receipts = user_list_create();
    uname(&ms->my_uname);
    
    if (send_formatted(fd, "220 %s Service ready\r\n", ms->my_uname.nodename) <= 0) return;

  
    while ((len = nb_read_line(ms->nb, ms->recvbuf)) >= 0) {
	if (ms->recvbuf[len - 1] != '\n') {
	    // command line is too long, stop immediately
	    send_formatted(fd, "500 Syntax error, command TOO long unrecognized\r\n");
	    break;
	}
	if (strlen(ms->recvbuf) < len) {
	    // received null byte somewhere in the string, stop immediately.
	    send_formatted(fd, "500 Syntax error, command unrecognized\r\n");
	    break;
	}
    
	// Remove CR, LF and other space characters from end of buffer
	while (isspace(ms->recvbuf[len - 1])) ms->recvbuf[--len] = 0;
    
	dlog("Command is %s\n", ms->recvbuf);
    
	// Split the command into its component "words"
	ms->nwords = split(ms->recvbuf, ms->words);
	char *command = ms->words[0];
    
	if (!strcasecmp(command, "QUIT")) {
	    if (do_quit(ms) == -1) break;
	} else if (!strcasecmp(command, "HELO") || !strcasecmp(command, "EHLO")) {
	    if (do_helo(ms) == -1) break;
	} else if (!strcasecmp(command, "MAIL")) {
	    if (do_mail(ms) == -1) break;
	} else if (!strcasecmp(command, "RCPT")) {
	    if (do_rcpt(ms) == -1) break;
	} else if (!strcasecmp(command, "DATA")) {
	    if (do_data(ms) == -1) break;
	} else if (!strcasecmp(command, "RSET")) {
	    if (do_rset(ms) == -1) break;
	} else if (!strcasecmp(command, "NOOP")) {
	    if (do_noop(ms) == -1) break;
	} else if (!strcasecmp(command, "VRFY")) {
	    if (do_vrfy(ms) == -1) break;
	} else if (!strcasecmp(command, "EXPN") ||
		   !strcasecmp(command, "HELP")) {
	    dlog("Command not implemented \"%s\"\n", command);
	    if (send_formatted(fd, "502 Command not implemented\r\n") <= 0) break;
	} else {
	    // invalid command
	    dlog("Illegal command \"%s\"\n", command);
	    if (send_formatted(fd, "500 Syntax error, command unrecognized\r\n") <= 0) break;
	}
    }
  
    nb_destroy(ms->nb);
}
