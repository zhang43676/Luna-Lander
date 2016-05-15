/* name:               Zhao Zhang
 * ONE Card number:    1271797 
 * Unix id:            zz9
 * lecture section:    A1
 * instructor's name   Martin Mueller
 * lab section         D04
 * TA's name           Ding Liu
 * 
 * This program will now lunar lander game. 
 * 
 *           
 */
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <math.h>
#include <ctype.h>
#include <getopt.h>
#include <curses.h>

#define PI                  acos(-1.0)

//functions and structs
typedef struct __landerLocation LanderLocationCoords;
typedef struct {
	char lineString[257];
} Line;
typedef struct __ship_figure ship_Figure;
struct BuildMap  {
    double dx[30],dy[30];
    int drawCount;
};
struct __landerLocation {
    double x,y;
    double dx,dy;
};
struct __ship_figure {
    char name[15];
    double side_length;
    double x_offset, y_offset;
    double x_offset_old, y_offset_old;

    double velocity_x,velocity_y;
    double thrust_x,thrust_y;
    bool thrust_on;
    double terminal_velocity;
    double orientation;
    LanderLocationCoords drawship[200];
    int segmentCount;
};

ship_Figure* newship ( char * name, double side_length,
                          double x_offset, double y_offset );
void initLanderFigure ( ship_Figure * ship );
void updateShip ( ship_Figure * ship, double x_shift, double y_shift,
                    int rotate_direction );
void drawship ( ship_Figure * ship );
void eraseShip ( ship_Figure * ship );
void rotate_ship ( ship_Figure * ship_at_begin, ship_Figure * ship,
                    int rotate_direction );
void get_Thrust ( ship_Figure * ship  );

bool lineSegmentIntersection(
double Ax, double Ay,
double Bx, double By,
double Cx, double Cy,
double Dx, double Dy,
double *X, double *Y);

void DrawThrust ( ship_Figure * ship );
void EraseThrust ( ship_Figure * ship  );
void OpenThrust ( ship_Figure * ship  );
void setLandscape ( struct BuildMap * landscape );
void drawLandscape ( void );
void startTimer ( void * pFunc);
void stopTimer ( void );
void handle_timeout ( int sig );
void InitLandscape(FILE * inputFile);
ship_Figure* ship_fly(void);
ship_Figure* getbaseLander(void);

void sket_Draw(long int x1, long int y1 , long int x2 , long int y2);
void sket_Erase(long int x1, long int y1 , long int x2 , long int y2);
void take_out_space (char * lineString);
//global values
struct itimerval g_itimer;
struct timeval g_timer_inter;
struct timeval g_timer_start;
static double g_velocity_threshold = 35;
static double gravity_start= 0.0;
static double thrusts_start = -0.0;
static sigset_t ship_Mask;
static int g_input_key=0;
static int g_n_landers = 0;
static int ship_status = 0;
static FILE *in_sket = NULL;
static struct BuildMap landscape_segments;
static struct BuildMap * g_pLandscape=NULL;
static ship_Figure * g_ship_at_begin=NULL;
static ship_Figure * g_ctrlLander=NULL;
//get the node of lander
ship_Figure* ship_fly(void){
	return g_ctrlLander;
}
ship_Figure* getbaseLander(void){
	return g_ship_at_begin;
}
//start the timer
void startTimer (void *pFunc) {
	g_timer_inter.tv_usec = 50000;
	g_timer_inter.tv_sec = 0.05;
	g_timer_start.tv_usec = 50000;
	g_timer_start.tv_sec = 0.05;
	g_itimer.it_interval = g_timer_inter;
	g_itimer.it_value = g_timer_start;
	setitimer(ITIMER_REAL, &g_itimer, 0);
	struct sigaction handleTimeStep;
	handleTimeStep.sa_handler = pFunc;
	sigemptyset(&handleTimeStep.sa_mask);
	handleTimeStep.sa_flags = 0;
	sigaction(SIGALRM, &handleTimeStep, 0);
}
// stop the timer
void stopTimer ( void ) {
	g_timer_inter.tv_usec = 0;
	g_timer_inter.tv_sec = 0;
	g_timer_start.tv_usec = 0;
	g_timer_start.tv_sec = 0;
	g_itimer.it_interval = g_timer_inter;
	g_itimer.it_value = g_timer_start;
	setitimer(ITIMER_REAL, &g_itimer, 0);
}
//draw landscape
void initLanderFigure ( ship_Figure * ship ) {
	double side_length = ship->side_length;
	double height = side_length;

	double x_offset = ship->x_offset;
	double y_offset = ship->y_offset;
	ship->segmentCount = 4;
	ship->drawship[0].x = - side_length/4;
	ship->drawship[0].y = - height/2;
	ship->drawship[0].dx = side_length/4;
	ship->drawship[0].dy = - height/2;
	ship->drawship[1].x = side_length/4;
	ship->drawship[1].y = - height/2;
	ship->drawship[1].dx = side_length/2;
	ship->drawship[1].dy = height/2;
	ship->drawship[2].x = side_length/2;
	ship->drawship[2].y = height/2;
	ship->drawship[2].dx = - side_length/2;
	ship->drawship[2].dy = height/2;
	ship->drawship[3].x = - side_length/2;
	ship->drawship[3].y = height/2;
	ship->drawship[3].dx = - side_length/4;
	ship->drawship[3].dy = (- height/2);
	sket_Draw(
		lround(- side_length/4 + x_offset),
		lround(- height/2 + y_offset),
		lround(side_length/4 + x_offset),
		lround(- height/2 + y_offset ));
	sket_Draw(  
		lround(side_length/4 + x_offset),
		lround(- height/2 + y_offset),
		lround(side_length/2 + x_offset),
		lround(height/2 + y_offset));
	sket_Draw(  
		lround(side_length/2 + x_offset),
		lround(height/2 + y_offset),
		lround(- side_length/2 + x_offset),
		lround(height/2 + y_offset));
	sket_Draw(  
		lround(- side_length/2 + x_offset),
		lround(height/2 + y_offset ),
		lround(- side_length/4 + x_offset),
		lround(- height/2 + y_offset ));
}
//start a newship
ship_Figure* newship ( char * name, double side_length,double x_offset, double y_offset ) {
	if ( g_n_landers == 0 ) {
             sigemptyset(&ship_Mask);
	     sigaddset(&ship_Mask, SIGALRM);
        }
	ship_Figure * ship = calloc(1, sizeof(ship_Figure));
        strcpy(ship->name, name);
	ship->side_length = side_length;
	ship->x_offset = x_offset;
        ship->y_offset = y_offset;
	ship->x_offset_old = x_offset;
	ship->y_offset_old = y_offset;
	ship->velocity_x = 0.0;
	ship->velocity_y = 0.0;
	ship->thrust_x = 0.0;
        ship->thrust_y = 0.0;
	ship->thrust_on = false;
	ship->terminal_velocity = sqrt( 2.0 * gravity_start * (480 - y_offset) );
	ship->orientation = 90.00;
	memset(ship->drawship, 0, sizeof(LanderLocationCoords) * 200);
	g_n_landers++;
	initLanderFigure(ship);
	return ship;
}
// update ship shift coodornation
void updateShip ( ship_Figure * ship, double x_shift, double y_shift,int rotate_direction ){
        eraseShip(ship);
	ship->x_offset += x_shift;
	ship->y_offset += y_shift;
	ship_Figure * ship_at_begin = getbaseLander();
	rotate_ship(ship_at_begin, ship, rotate_direction);
	drawship(ship);
}
// 
void drawship ( ship_Figure * ship ) {
	double x_offset = ship->x_offset;
	double y_offset = ship->y_offset;
	int i;
	for ( i = 0; i < ship->segmentCount; i++ ) {
		sket_Draw(
			lround( ship->drawship[i].x + x_offset ),
			lround( ship->drawship[i].y + y_offset ),
			lround( ship->drawship[i].dx + x_offset ),
			lround( ship->drawship[i].dy + y_offset ));
	}
}
// erase ship 
void eraseShip ( ship_Figure * ship ) {
	double x_offset = ship->x_offset;
	double y_offset = ship->y_offset;
	int i;
	for ( i = 0; i < ship->segmentCount; i++ ) {
		sket_Erase(
			lround( ship->drawship[i].x + x_offset ),
			lround( ship->drawship[i].y + y_offset ),
			lround( ship->drawship[i].dx + x_offset ),
			lround( ship->drawship[i].dy + y_offset ));
	}
}
// raotate ship
void rotate_ship ( ship_Figure * ship_at_begin, ship_Figure * ship, int rotate_direction ) {
	double angle = 10;
	ship_at_begin->orientation = ( (long) ( ship_at_begin->orientation +
		rotate_direction*angle )) % 360;
	double radians = ((ship_at_begin->orientation - 90.0) * (PI)) / 180.0;
	int segmentCount = ship->segmentCount;
	double x, y, rx, ry;
	int i;
	for ( i = 0; i < segmentCount; i++ ) {
		x = ship_at_begin->drawship[i].x;
		y = ship_at_begin->drawship[i].y;
		rx = (x * cos(radians)) - (sin(radians) * y);
		ry = (x * sin(radians)) + (cos(radians) * y);
		ship->drawship[i].x = rx;
		ship->drawship[i].y = ry;
		x = ship_at_begin->drawship[i].dx;
		y = ship_at_begin->drawship[i].dy;
		rx = (x * cos(radians)) - (sin(radians) * y);
		ry = (x * sin(radians)) + (cos(radians) * y);
		ship->drawship[i].dx = rx;
		ship->drawship[i].dy = ry;
	}
}
//compute velocity of thrust
void get_Thrust ( ship_Figure * ship ) {
	ship_Figure * ship_at_begin = getbaseLander();
	double orientation = ship_at_begin->orientation;
	double rad_orientation = (PI * orientation)/180;
	double thrust_y = gravity_start + (thrusts_start * sin(rad_orientation));
	double velocity_y = ship->velocity_y + (thrust_y * 0.05);
	ship->velocity_y = velocity_y;
	ship->thrust_y = thrust_y;
	double thrust_x = thrusts_start * cos(rad_orientation);
	double velocity_x = ship->velocity_x + (thrust_x * 0.05);
	ship->velocity_x = velocity_x;
	ship->thrust_x = thrust_x;
}
//draw thrust
void DrawThrust ( ship_Figure * ship) {
	ship_Figure * ship_at_begin = getbaseLander();
	double orientation = ship_at_begin->orientation;
	double radians = ((orientation - 90.0) * (PI)) / 180.0;
	double thrustFigure_x = 0;
	double thrustFigure_y = (3.0/4.0) * (ship_at_begin->side_length);
	double rotatedThrustFigure_x = (thrustFigure_x * cos(radians)) -
		(thrustFigure_y * sin(radians));
	double rotatedThrustFigure_y = (thrustFigure_x * sin(radians)) +
		(thrustFigure_y * cos(radians));
	sket_Draw(
		lround( ship->drawship[2].x + ship->x_offset ),
		lround( ship->drawship[2].y + ship->y_offset ),
		lround( rotatedThrustFigure_x + ship->x_offset ),
		lround( rotatedThrustFigure_y + ship->y_offset ));
	sket_Draw(
		lround( rotatedThrustFigure_x + ship->x_offset ),
		lround( rotatedThrustFigure_y + ship->y_offset ),
		lround( ship->drawship[2].dx + ship->x_offset ),
		lround( ship->drawship[2].dy + ship->y_offset ));
}
//erase thrust
void EraseThrust ( ship_Figure * ship ) {
	ship_Figure * ship_at_begin = getbaseLander();
	double orientation = ship_at_begin->orientation;
	double rotatedThrustFigure_x = (0 * cos(((orientation - 90.0) * (PI)) / 180.0)) -
		((3.0/4.0) * (ship_at_begin->side_length) * sin(((orientation - 90.0) * (PI)) / 180.0));
	double rotatedThrustFigure_y = (0 * sin(((orientation - 90.0) * (PI)) / 180.0)) +
		((3.0/4.0) * (ship_at_begin->side_length) * cos(((orientation - 90.0) * (PI)) / 180.0));
	sket_Erase(
		lround( ship->drawship[2].x + ship->x_offset ),
		lround( ship->drawship[2].y + ship->y_offset ),
		lround( rotatedThrustFigure_x + ship->x_offset ),
		lround( rotatedThrustFigure_y + ship->y_offset ));
	sket_Erase(
		lround( rotatedThrustFigure_x + ship->x_offset ),
		lround( rotatedThrustFigure_y + ship->y_offset ),
		lround( ship->drawship[2].dx + ship->x_offset ),
		lround( ship->drawship[2].dy + ship->y_offset ));
}
//open thrust when space press
void OpenThrust ( ship_Figure * ship ) {
	ship->thrust_on = true;
}
// check the intersection
bool lineSegmentIntersection(
double Ax, double Ay,
double Bx, double By,
double Cx, double Cy,
double Dx, double Dy,
double *X, double *Y) {

  double  distAB, theCos, theSin, newX, ABpos ;

  //  Fail if either line segment is zero-length.
  if (Ax==Bx && Ay==By || Cx==Dx && Cy==Dy) return false;

  //  Fail if the segments share an end-point.
  if (Ax==Cx && Ay==Cy || Bx==Cx && By==Cy
  ||  Ax==Dx && Ay==Dy || Bx==Dx && By==Dy) {
    return false; }

  //  (1) Translate the system so that point A is on the origin.
  Bx-=Ax; By-=Ay;
  Cx-=Ax; Cy-=Ay;
  Dx-=Ax; Dy-=Ay;

  //  Discover the length of segment A-B.
  distAB=sqrt(Bx*Bx+By*By);

  //  (2) Rotate the system so that point B is on the positive X axis.
  theCos=Bx/distAB;
  theSin=By/distAB;
  newX=Cx*theCos+Cy*theSin;
  Cy  =Cy*theCos-Cx*theSin; Cx=newX;
  newX=Dx*theCos+Dy*theSin;
  Dy  =Dy*theCos-Dx*theSin; Dx=newX;

  //  Fail if segment C-D doesn't cross line A-B.
  if (Cy<0. && Dy<0. || Cy>=0. && Dy>=0.) return false;

  //  (3) Discover the position of the intersection point along line A-B.
  ABpos=Dx+(Cx-Dx)*Dy/(Dy-Cy);

  //  Fail if segment C-D crosses line A-B outside of segment A-B.
  if (ABpos<0. || ABpos>distAB) return false;

  //  (4) Apply the discovered position to line A-B in the original coordinate system.
  *X=Ax+ABpos*theCos;
  *Y=Ay+ABpos*theSin;

  //  Success.
  return true; }
void handle_timeout ( int sig ) {
	sigset_t oldMask;
	sigprocmask(SIG_BLOCK, &ship_Mask, &oldMask);
	ship_Figure*ship = ship_fly();
	ship->thrust_on = false;
	EraseThrust(ship);
	ship->thrust_x = 0.0;
	ship->thrust_y = 0.0;
	g_input_key =  wgetch(stdscr);
	switch(g_input_key){
		case KEY_LEFT:
			updateShip(ship_fly(), 0, 0, -1);
			break;
		case KEY_RIGHT:
			updateShip(ship_fly(), 0, 0, 1);
			break;
		case 0x20:
			get_Thrust(ship_fly());
			OpenThrust(ship_fly());
			break;
	}
	double velocity_x = 0,velocity_y = 0,x_offset_new = 0.0;
	double x_offset_old = ship->x_offset;
	ship->x_offset_old = x_offset_old;
	x_offset_new = x_offset_old +
		ship->velocity_x * 0.05 +
		0.5 * ship->thrust_x * pow(0.05, 2);
	velocity_x = ship->velocity_x + (ship->thrust_x * 0.05);
	double x_offset_diff = x_offset_new - x_offset_old;
	ship->velocity_x = velocity_x;
	double y_offset_old = ship->y_offset;
	ship->y_offset_old = y_offset_old;
	double y_offset_new = 0.0;
	if ( ship->velocity_y < ship->terminal_velocity ) {
		y_offset_new = y_offset_old +
			ship->velocity_y * 0.05 +
			0.5 * gravity_start * pow(0.05, 2);
		velocity_y = ship->velocity_y + ((ship->thrust_y + gravity_start) * 0.05);
		ship->velocity_y = velocity_y;
	}
	else {
		y_offset_new = y_offset_old +
			ship->velocity_y * 0.05 +
			0.5 * gravity_start * pow(0.05, 2);
	}
	double y_offset_diff = y_offset_new - y_offset_old;
	updateShip(ship, x_offset_diff, y_offset_diff, 0);
	if ( ship->thrust_on != false )
		DrawThrust(ship);

	ship_Figure * ship_at_begin = getbaseLander();
	bool intersected = false;
	double ship_x1, ship_y1,ship_x2, ship_y2,landscape_x1, landscape_y1,landscape_x2, landscape_y2,intersect_x, intersect_y;
	intersect_x = -1.0;
	intersect_y = -1.0;
	int i, j;
	for ( i = 0; i < ship->segmentCount; i++ ) {
		for ( j = 0; j < g_pLandscape->drawCount-1; j++ ) {
			ship_x1 = ship->drawship[i].x + x_offset_new;
			ship_y1 = ship->drawship[i].y + y_offset_new;
			ship_x2 = ship->drawship[i].dx + x_offset_new;
			ship_y2 = ship->drawship[i].dy + y_offset_new;
			landscape_x1 = g_pLandscape->dx[j];
			landscape_y1 = g_pLandscape->dy[j];
			landscape_x2 = g_pLandscape->dx[j+1];
			landscape_y2 = g_pLandscape->dy[j+1];
			intersected = lineSegmentIntersection(ship_x1, ship_y1,ship_x2,ship_y2,landscape_x1,landscape_y1,landscape_x2,landscape_y2,&intersect_x,&intersect_y );				
			if ( intersected == true ) {
				if ( landscape_y1 != landscape_y2 ||
					velocity_y > g_velocity_threshold ||
					velocity_x > g_velocity_threshold ||
					ship_at_begin->orientation != 90.00 )
					ship_status = -1;
				else
					ship_status = 1;
			}
		}
	}
	sigprocmask(SIG_SETMASK, &oldMask, NULL);
}
static Line* initLines ( int size ) {
	Line * LinePtr;
	LinePtr = calloc((size_t) size, sizeof(Line));

	return LinePtr;
}
static int readFile(Line * Lines, FILE * inputFile) {
	if ( ftell(inputFile) != 0 ) {
		fseek(inputFile, 0L, SEEK_SET);
	}
	int currentLine = 0;
	while (fgets( Lines[currentLine].lineString, 257, inputFile)
		!= NULL) {
			take_out_space(Lines[currentLine].lineString);
			currentLine++;
	}
	if ( !currentLine )
		return 1;
	return 0;
}
//initlized landscape when run it at very first time
void InitLandscape(FILE * inputFile){
        char * point_x_token;
	char * point_y_token;
	double point_x,point_y;
	int currentLine;
	int inputFileLength = 0;
	landscape_segments.drawCount = 0;
	char * buff = (char *) malloc(257);
	memset(buff, '\0', (257));
	for ( inputFileLength = 0; 
		fgets(buff, 257, inputFile) != NULL;
		inputFileLength++ );
	free(buff);
	Line * Lines = initLines(inputFileLength);
	readFile(Lines, inputFile);
	for ( currentLine = 0; currentLine < inputFileLength; currentLine++ ) {
		point_x_token = strtok(Lines[currentLine].lineString, " ");
		point_y_token = strtok(NULL, " ");
		point_x = strtod(point_x_token, NULL);
		point_y = strtod(point_y_token, NULL);
		if ( landscape_segments.drawCount < 30 ) {
			landscape_segments.dx[landscape_segments.drawCount] = point_x;
			landscape_segments.dy[landscape_segments.drawCount] = point_y;
			landscape_segments.drawCount++;
		}
	}
	g_pLandscape = &landscape_segments;
		int i;
	for ( i = 0; i < g_pLandscape->drawCount-1; i++ )
		sket_Draw(
		lround( g_pLandscape->dx[i] ),
		lround( g_pLandscape->dy[i] ),
		lround( g_pLandscape->dx[i+1] ),
		lround( g_pLandscape->dy[i+1] ) );
}
//take out of the space
void take_out_space (char * String) {
	int len = strlen(String);
	int offset = 1;
	while ( isspace( *(String + len - offset) ))
		offset++;
	*(String+len-offset+1) = '\0';
}
//draw to the sketchpad
void sket_Draw(long int x1, long int y1 , long int x2 , long int y2){
	FILE * sket = in_sket;
	fprintf(sket, "drawSegment %ld %ld %ld %ld\n",  x1, y1, x2, y2);
	fflush(sket);
}
//erase sketchpad
void sket_Erase(long int x1, long int y1 , long int x2 , long int y2){
	FILE * sket = in_sket;
	fprintf(sket, "eraseSegment %ld %ld %ld %ld\n",  x1, y1, x2, y2);
	fflush(sket);
}
//main functions
int main ( int argc, char * argv[] ) {
	char opt;
	double input_gravity = 0,input_thrust = 0;
	FILE * read_file = NULL;

	opt = getopt(argc, argv, "g:t:f:i");
	while(opt!=-1){
		switch(opt){
			case 'g':
				input_gravity = strtod(optarg, NULL);
				if ( input_gravity < 0.0 || input_gravity > 20 ) {
					printf("error: gravity < 0, > 20 is not allowed\n");
					exit(-1);
				}
				gravity_start = input_gravity;
				break;

			case 't':
				input_thrust = strtod(optarg, NULL);
				if ( input_thrust > 0.0 || input_thrust < -20 ) {
					printf("error: thrust > 0, < -20 is not allowed\n");
					exit(-1);
				}
				thrusts_start = input_thrust;
				break;

			case 'f':
				read_file = fopen(optarg, "r");
				if ( read_file ==  NULL ) {
					printf("could not open file %s\n", optarg);
					exit(-1);
				}
				break;                        
		}
		opt = getopt(argc, argv, "g:t:f:i");
	}
	in_sket = popen("java -jar Sketchpad.jar",  "w");
	initscr();
	move(5, 10);
	printw("Press any key to start.");
	move(6, 10);
	printw("(Then press arrow keys to rotate, space for thrust, 'q' to quit.)");
	refresh();
	getch();
	InitLandscape(read_file);
	g_ship_at_begin =  newship("start",20.0,320.0,10.0);
	eraseShip(g_ship_at_begin);
	g_ctrlLander =  newship("control",20.0,320.0,10.0);
	startTimer((void*)handle_timeout);
	clear();
	move(5, 10);
	printw("left arrow key rotates counter-clockwise, right clockwise, "
		" space for thrust, q to quit.");
	refresh();
	keypad(stdscr, true);
	nodelay(stdscr, true);
	noecho();
	for ( ; g_input_key != 'q'; )
	{
		if ( ship_status != 0 )
			break;
	}
	stopTimer();
	move(7, 10);
	if (ship_status == -1 ) {
           printw("CRASHED!!!");
	   nodelay(stdscr, false);
	}
	else if (ship_status == 1 ) {
           printw("LANDED!!!!");
	   nodelay(stdscr, false);
	}
	while ( getch() != 'q' ){};
		endwin();
        fprintf(in_sket, "end\n");
	pclose(in_sket);
        exit(-1);
	return 0;
}
