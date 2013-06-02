#include <time.h>
#include <stdio.h>
#include <sys/time.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <gtk/gtk.h>

//control flag for the signal
int done; 
static int loop;

//global thread object
pthread_t myThread;

//size of string buffer
#define BUFCOUNT 64

//gtk window made global for easy access
GtkWidget *window;

//global gtk labels for timeout() to access
GtkWidget *proc_label_text;
GtkWidget *sys_label_text;

//simple structure to pass gui elements
typedef struct windowInfo {
	GtkWidget *start;
	GtkWidget *startText;
	GtkWidget *endText;
	GtkWidget *progressText;
} windowInfo;

//signal number
#define SIG_TEST 44 

//signal handler function
void receiveData(int n, siginfo_t *info, void *unused) {
	char message[BUFCOUNT];
	sprintf(message, "Signal handler received value %i\n", info->si_int);
	
	GtkWidget* popwindow;
	GtkWidget* text;
	
	//make a new popup window
	popwindow = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title (GTK_WINDOW (popwindow), "Alarm");
	
	text = gtk_label_new(message);
	gtk_container_add(GTK_CONTAINER(popwindow), text);
	
	//show the window
	gtk_widget_show_all (popwindow);
}

//the main function from the previous assignments.
//no longer main here, just a regular old function
void* old_main(void* lpParam) 
{
	int	n, i, INTERVAL = 1000;
	
	char startbuff[BUFCOUNT];
	char endbuff[BUFCOUNT];
	
	void 	timeout();
	struct timeval start, end;
	struct	itimerval	myvalue, myovalue;
	struct sigaction sig;
	windowInfo* info;
	
	//memset our buffers
	memset(startbuff, 0, BUFCOUNT);
	memset(endbuff, 0, BUFCOUNT);
	
	//get gui info 
	info = (windowInfo*) lpParam;
	
	//stop the button from being pressed until we are done
	gtk_widget_set_sensitive(info->start,FALSE);
	
	//setup our signal handler for the alarm from the kernel
	sig.sa_sigaction = receiveData;
	sig.sa_flags = SA_SIGINFO;
	sigaction(SIG_TEST, &sig, NULL);	
	
	//number of loops so that we follow the cpu clock 
	int loops_to_do = 300*(1000/INTERVAL);
		
	if (gettimeofday(&start, NULL) == -1) {
		perror("Could not get the time");
		exit(-1);
	}
	
	//output the start time
	sprintf(startbuff, "%s", ctime(&start.tv_sec));
	gtk_label_set_text(GTK_LABEL(info->startText), startbuff);
		
	//show we have started
	gtk_label_set_text(GTK_LABEL(info->progressText), "In Progress");
			
	for (i = 0; i < loops_to_do; i++) {
		//set up the signal
		if ((signal(SIGALRM, timeout)) == SIG_ERR) {
			printf("Error in setting signal handler\n");
			exit(-1);
		}
		//set our timer
		timerclear(&myvalue.it_interval);
		timerclear(&myvalue.it_value);
		timerclear(&myovalue.it_interval);
		timerclear(&myovalue.it_value);
		if (( n = setitimer(ITIMER_REAL,&myvalue,&myovalue)) < 0 ) {
			printf("Error in clearing timer\n");
			exit(-1);
		}
			
		//set the interval to output the signal
		if (INTERVAL == 1000) {
			myvalue.it_value.tv_sec = 1; /* timeout interval in seconds */
			myvalue.it_value.tv_usec = 0; /* set tv_usec for microsecond timeout */
		} else {
			myvalue.it_value.tv_sec = 0;
			myvalue.it_value.tv_usec = 1000*INTERVAL; /* set tv_usec for microsecond timeout */
		}
		if (( n = setitimer(ITIMER_REAL,&myvalue,&myovalue)) < 0 ) {
			printf("Error in setting timer\n");
			exit(0);
		}	
	
		//don't go to the next interation of 
		//the loop until we handle the signal
		done = 0;
		while(!done);
	}
	
	if (gettimeofday(&end, NULL) == -1){
		perror("Could not get the time");
		exit(-1);
	}
		
	//output the start and end time. 	
	sprintf(endbuff, "%s", ctime(&end.tv_sec));
	gtk_label_set_text(GTK_LABEL(info->endText), endbuff);
	
	//show that we are done to the gui
	gtk_label_set_text(GTK_LABEL(info->progressText), "Done");
	gtk_widget_set_sensitive(info->start,TRUE);
	
	pthread_exit(NULL);
}

void timeout()
{	
	int fd; 
	char buffer[BUFCOUNT];
	char procBuffer[BUFCOUNT];
	char sysBuffer[BUFCOUNT];
	time_t current_time1, current_time2;
	int sec, usec1, usec2; 
	
	//-----------------------------------------------------------
	//read the proc file
	//-----------------------------------------------------------
	
	memset(buffer, 0, BUFCOUNT);
	memset(procBuffer, 0,BUFCOUNT);
	memset(sysBuffer, 0,BUFCOUNT);
	
	//open the psuedo file
	fd = open("/proc/myclock", O_RDONLY);
	if (fd == -1) {
		perror("Could not open /proc/myclock psuedo file");
		exit(-1);
	}
		
	//read from the file
	if (read(fd, &buffer[0], BUFCOUNT) == -1) {
		perror("Could not read from /proc/myclock psuedo file");
		exit(-1);
	}
	
	//close the file
	if (close(fd) == -1) {
		perror("Could not close /proc/myclock psuedo file");
		exit(-1);
	}
	
	//extract the time in seconds and microseconds.
	sscanf(&buffer[0], "%d %d", &sec,&usec1);
	current_time1 = (time_t)sec;
	
	//-----------------------------------------------------------
	//once again for the sys file
	//-----------------------------------------------------------
	
	memset(buffer, 0, BUFCOUNT);
	
	//open the psuedo file
	fd = open("/sys/kernel/myclock2/myclock2", O_RDONLY);
	if (fd == -1) {
		perror("Could not open /sys/kernel/myclock2/myclock2 psuedo file");
		exit(-1);
	}
		
	//read from the file
	if (read(fd, &buffer[0], BUFCOUNT) == -1) {
		perror("Could not read from /sys/kernel/myclock2/myclock2 psuedo file");
		exit(-1);
	}
	
	//close the file
	if (close(fd) == -1) {
		perror("Could not close /sys/kernel/myclock2/myclock2 psuedo file");
		exit(-1);
	}
	
	//extract the time in seconds and microseconds.
	sscanf(&buffer[0], "%d %d", &sec,&usec2);
	current_time2 = (time_t)sec;
	
	//change this
	sprintf(procBuffer, "%s", ctime(&current_time1));
	sprintf(sysBuffer, "%s", ctime(&current_time2));
	
	gtk_label_set_text(GTK_LABEL(proc_label_text), procBuffer);
	gtk_label_set_text(GTK_LABEL(sys_label_text), sysBuffer);
	
	done = 1;
}

//gui callback function for starting the program
void StartThread(GtkWidget *widget, gpointer info) {
	if(pthread_create(&myThread, NULL, &old_main, (void*)info)) { 
		perror("Cound not create thread.\n");
		exit(-1);
	}
}

//gui callback for sending a signal
void SendSignal(GtkWidget *widget, gpointer pointer) {
	int	fd, time;
	char mybuff[BUFCOUNT];
	char * string;
	
	memset(mybuff, 0, BUFCOUNT);
	
	//get the time duration for the signal
	string = (char*)gtk_entry_get_text(GTK_ENTRY(pointer));
	time = atoi((char*)string);
		
	//open the file		
	fd = open("/sys/kernel/myclock2/setclock", O_RDWR);
	if (fd == -1) {
		perror("Could not open /sys/kernel/myclock2/setclock psuedo file");
		exit(-1);
	}
		
	//write to the file to set the alarm time
	sprintf(mybuff,"%d",time);
	if (write(fd, &mybuff[0], BUFCOUNT) == -1) {
		perror("Could not write to /sys/kernel/myclock2/setclock psuedo file");
		exit(-1);
	}
			
	//read from the file to start the alarm
	if (read(fd, &mybuff[0], BUFCOUNT) == -1) {
		perror("Could not read from /sys/kernel/myclock2/setclock psuedo file");
		exit(-1);
	}		
	
	//close the file
	if (close(fd) == -1) {
		perror("Could not close /sys/kernel/myclock2/setclock psuedo file");
		exit(-1);
	}
}

//new main function, entirely gui code
int main( int argc, char *argv[]) {
	//main container
	GtkWidget *vbox;
  
	//horizontal containers in vbox
	GtkWidget *hboxStart;
	GtkWidget *hboxSignal;
	GtkWidget *hboxProc;
	GtkWidget *hboxSys;
	GtkWidget *hboxStartTime;
	GtkWidget *hboxEndTime;
	GtkWidget *hboxProgress;
  
	GtkWidget *startButton;
	
	//widgets for sending a signal
	GtkWidget *signalButton;
	GtkWidget *signalEntry;
	GtkWidget *signalLabel;
	
	//some labels
	GtkWidget *proc_label;
	GtkWidget *sys_label;
  
    //widgets for displaying start and end time
	GtkWidget *start_time_label;
	GtkWidget *start_time_label_text;
	GtkWidget *end_time_label;
	GtkWidget *end_time_label_text;

	GtkWidget *progess_label;
	GtkWidget *progress_label_text;

	windowInfo info; //struct to pass
  
	gtk_init(&argc, &argv);

	//set up window
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_default_size(GTK_WINDOW(window), 350, 200);
  
	vbox = gtk_vbox_new(TRUE, 1);
	gtk_container_add(GTK_CONTAINER(window), vbox);
  
    //set up individual rows. 
	hboxStart = gtk_hbox_new(TRUE, 1);
	hboxSignal = gtk_hbox_new(TRUE, 1);
	hboxProc = gtk_hbox_new(TRUE, 1);
	hboxSys = gtk_hbox_new(TRUE, 1);
	hboxStartTime = gtk_hbox_new(TRUE, 1);
	hboxEndTime = gtk_hbox_new(TRUE, 1);
	hboxProgress = gtk_hbox_new(TRUE, 1);
  
	gtk_box_pack_start(GTK_BOX(vbox), hboxStart, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hboxSignal, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hboxProc, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hboxSys, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hboxStartTime, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hboxEndTime, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hboxProgress, TRUE, TRUE, 0);
  
	//create all the widgets
	startButton = gtk_button_new_with_label("Start 5 minute loop");
	signalLabel = gtk_label_new("Alarm (in seconds):");
	signalButton = gtk_button_new_with_label("Send Alarm");
	signalEntry = gtk_entry_new();
  
	proc_label = gtk_label_new("Proc Entry:");
	sys_label = gtk_label_new("Sys Entry:"); 
	proc_label_text = gtk_label_new(" ");
	sys_label_text = gtk_label_new(" ");
  
	start_time_label = gtk_label_new("Start Time:");
	end_time_label = gtk_label_new("End Time:");
	start_time_label_text = gtk_label_new(" ");
	end_time_label_text = gtk_label_new(" ");
  
	progess_label = gtk_label_new("Progress:");
	progress_label_text = gtk_label_new("Not started");
  
    //place all the widgets into the rows
	gtk_box_pack_start(GTK_BOX(hboxStart), startButton, TRUE, TRUE, 0);
  
	gtk_box_pack_start(GTK_BOX(hboxSignal), signalLabel, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hboxSignal), (GtkWidget*)signalEntry, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hboxSignal), signalButton, TRUE, TRUE, 0);
  
  
	gtk_box_pack_start(GTK_BOX(hboxProc), proc_label, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hboxProc), proc_label_text, TRUE, TRUE, 0);
  
	gtk_box_pack_start(GTK_BOX(hboxSys), sys_label, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hboxSys), sys_label_text, TRUE, TRUE, 0);
  
	gtk_box_pack_start(GTK_BOX(hboxStartTime), start_time_label, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hboxStartTime), start_time_label_text, TRUE, TRUE, 0);
  
	gtk_box_pack_start(GTK_BOX(hboxEndTime), end_time_label, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hboxEndTime), end_time_label_text, TRUE, TRUE, 0);
  
	gtk_box_pack_start(GTK_BOX(hboxProgress), progess_label, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hboxProgress), progress_label_text, TRUE, TRUE, 0);
  
    //set up gui passing structure
	info.start = startButton;
	info.startText = start_time_label_text;
	info.endText = end_time_label_text;
	info.progressText = progress_label_text;
  
    //show window
	gtk_widget_show_all(window);

	//setup button callbacks 
	g_signal_connect_swapped(G_OBJECT(window), "destroy",
		G_CALLBACK(gtk_main_quit), NULL);
     
	g_signal_connect(startButton, "clicked", 
		G_CALLBACK(StartThread), &info);
		
	g_signal_connect(signalButton, "clicked", 
		G_CALLBACK(SendSignal), signalEntry);

	//START!!!
	gtk_main();

  return 0;
}