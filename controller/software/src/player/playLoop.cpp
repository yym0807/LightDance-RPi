#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstring>
#include <thread>
#include <vector>
#include <sstream>

#include "LEDPlayer.h"
#include "OFPlayer.h"
#include "player.h"
#include "utils.h"
#include "state_machine.h"
#include "const.h"

#define MAXLEN 100

// enum CMD { C_PLAY, C_PAUSE, C_STOP, C_RESUME };
extern const std::string cmds[10];
extern std::thread led_loop, of_loop;
extern Player player;
extern LEDPlayer led_player;
extern OFPlayer of_player;
extern int dancer_fd;
extern string path;

int main(int argc, char *argv[]) {
    // create player_to_cmd
    if (mkfifo(FIFO_PLAYER_TO_CMD, 0666) == -1) {
        if (errno != EEXIST) {
            fprintf(stderr, "Cannot create %s\n", FIFO_PLAYER_TO_CMD);
        } else {
            fprintf(stderr, "%s already exists\n", FIFO_PLAYER_TO_CMD);
        }
    } else
        fprintf(stderr, "%s created\n", FIFO_PLAYER_TO_CMD);
    int rd_fd, n;
    if (mkfifo(FIFO_CMD_TO_PLAYER, 0666) == -1) {
        if (errno != EEXIST) {
            fprintf(stderr, "Cannot create %s\n", FIFO_CMD_TO_PLAYER);
        } else {
            fprintf(stderr, "%s already exists\n", FIFO_CMD_TO_PLAYER);
        }
    } else
        fprintf(stderr, "%s created\n", FIFO_CMD_TO_PLAYER);
    rd_fd = open(FIFO_CMD_TO_PLAYER, O_RDONLY | O_NONBLOCK);
    if (rd_fd == -1) perror("open");
    char cmd_buf[MAXLEN];
    // of_playing = false;
    // led_playing = false;
    // timeval playedTime;
    StateMachine* fsm = new StateMachine();

    while (1) {     
        n = read(rd_fd, cmd_buf, MAXLEN);
        if (n > 0) { // command received
            stringstream ss(cmd_buf);
            string cmd;
            ss >> cmd;
            fprintf(stderr,"[playLoop] parsing command\n");
	        EVENT event = parse_event(cmd.c_str());
            fprintf(stderr, "[playLoop] cmd_buf: %s, event: %d\n", cmd_buf, event);
            
            FILE *fifo = fopen(FIFO_PLAYER_TO_CMD, "w");
            if(event == EVENT_NULL){
                fprintf(fifo, "Unknown command: %s", cmd.c_str());
                continue;
            }
            if(event == EVENT_PLAY)
            {
                string flag;
                for(; ss >> flag; !ss.eof())
                {
                    if(flag == "-ss")
                    {
                        long start;
                        ss >> start;
                        fprintf(fifo, "start: %ld", start);
                    }
                    else if(flag == "-to")
                    {
                        long end;
                        ss >> end;
                        fprintf(fifo, "end: %ld", end);
                    }
                    else if(flag == "-d")
                    {
                        long delay;
                        ss >> delay;
                        fprintf(fifo, "delay: %ld", delay);
                    }
                }
            }
            if(!fsm->processEvent(event))
            {
                fprintf(fifo, "%s failed", eventToStr(event).c_str());
                fprintf(fifo, "Current state: %s", stateToStr(fsm->getState()).c_str());
            }
            fclose(fifo);
        }
        else{
           fsm->execCurrState();
        }
    }
    close(rd_fd);
    return 0;
}
