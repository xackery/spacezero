/* spaceship format:
   first number: number of points,
   every next pair of numbers define a point.
   starting from the first point a line is drawed following the dots.
   the points must be inner the square (-1-1,1,1)
   The spaceship must be orientated west(rear)-east(front).
*/

float ship1[] = {3, -1, -0.6, 1, 0, -1, 0.6}; /*SHIP1 EXPLORER*/
float ship1b[] = {9,    -0.2, 0.2,  -1, 0.7,  0.6,  0.3,  0.9,  0.1, 0.9,
                  -0.1, 0.6,  -0.3, -1, -0.7, -0.2, -0.2, -0.2, 0.2};
float ship2[] = {5, -0.2, 0, -1, -0.6, 1, 0, -1, 0.6, -0.2, 0}; /* SHIP2 */
float ship3[] = {12,          /* number of points    SHIP3 FIGHTER */
                 0.866,  0.5, /* point coordenates */
                 0.5,    0.866,  -0.5, 0.866,  -0.866, 0.5,    0, 0,
                 -0.866, -0.5,   -0.5, -0.866, 0,      -0.866, 0, 0.866,
                 0,      -0.866, 0.5,  -0.866, 0.866,  -0.5};
float ship3b[] = {13,   -0.8, 0.1,  -0.1, 0.2,  -1.,  0.5,  -0.4, 1.0,
                  1.0,  0.4,  0.3,  0.2,  0.3,  -0.2, 1.0,  -0.4, -0.4,
                  -1.0, -1.,  -0.5, -0.1, -0.2, -0.8, -0.1, -0.8, 0.1};

float ship4[] = {12, /* SHIP4 QUEEN */
                 1,    0,     -0.5,  0.5,  -1,    0.5,   -0.5, 0.25,
                 -0.5, -0.25, -1,    -0.5, -0.5,  -0.5,  1,    0,
                 0.55, 0,     -0.25, 0.25, -0.25, -0.25, 0.55, 0};

float ship5[] = {6, /* MISSILE */
                 1, 0, 0.7, -0.3, -0.7, -0.3, -0.7, 0.3, 0.7, 0.3, 1, 0};

float ship6[] = {4, /* TOWER */
                 1., 0.5, 1.5, 0, 1., -0.5, 1., 0.5};

float ship7[] = {11, /* SHIP7 PILOT */
                 .25,  -.5,  .25,  .25,  -.25, .25, -.25, 0,   -1, 0,   -1,
                 -.25, -.25, -.25, -.25, -.5,  .75, -.5,  .75, 0,  .25, 0};

float ship8[] = {20, /* SHIP7 PILOT in pod */
                 0.75, 0.25, 0.75, .75,   .25, 1.25,  -.25, 1.25, -.75, .75,
                 -.75, -.75, -.25, -1.25, .25, -1.25, .75,  -.75, .75,  .25,
                 -.25, .25,  -.25, -.25,  0,   -.25,  0,    -1,   .25,  -1,
                 .25,  -.25, .5,   -.25,  .5,  .75,   0,    .75,  0,    .25};

float ship9[] = {18, /* SHIP7 PILOT in pod pirates*/
                 0.75, 0.25, 0.75, .75,  0,    1.3, -.75, .75,  -.75,
                 -.75, 0,    -1.3, .75,  -.75, .75, .25,  -.25, .25,
                 -.25, -.25, 0,    -.25, 0,    -1,  .25,  -1,   .25,
                 -.25, .5,   -.25, .5,   .75,  0,   .75,  0,    .25};

float ship10[] = {
    22, /* SHIP8 TRANSPORT */
    0.9,   0,    0.9,   0.15,  0.49,  0.25,  0.44,  0.53,  0.2,   0.65,  -1,
    0.38,  -0.5, 0.35,  -0.35, 0.30,  -0.4,  0.19,  -0.5,  0.1,   -0.8,  0.1,

    -0.8,  -0.1, -0.5,  -0.1,  -0.4,  -0.19, -0.35, -0.30, -0.5,  -0.35, -1,
    -0.38, 0.2,  -0.65, 0.44,  -0.53, 0.49,  -0.25, 0.9,   -0.15, 0.9,   0};

float ship10b[] = {18, /* SHIP8 TRANSPORT */
                   0.9,  0,    0.9,  0.1,  0.5,  0.2,  0.4,  0.7,  0.3,
                   0.8,  -1,   0.6,  -0.2, 0.5,  0,    0.2,  -0.8, 0.1,

                   -0.8, -0.1, 0,    -0.2, -0.2, -0.5, -1,   -0.6, 0.3,
                   -0.8, 0.4,  -0.7, 0.5,  -0.2, 0.9,  -0.1, 0.9,  0};

float ship11[] = {5, /* SHIP11 GOODS */
                  0.9, 0.9, -0.9, 0.9, -0.9, -0.9, 0.9, -0.9, 0.9, 0};

/*********** not used ************/

float shipx[] = {5, /* TOWER */
                 1., 0, 1., -0.5, 1.5, 0, 1., 0.5, 1., 0};

float ship3c[] = {15,   -0.8, 0.1,   0.1,  0.2,  -0.15, 0.5,   -1,
                  0.4,  0.4,  0.8,   0.5,  0.25, 1,     0.2,   1,
                  -0.2, 0.5,  -0.25, 0.4,  -0.8, -1,    -0.45, -0.15,
                  -0.5, 0.1,  -0.2,  -0.8, -0.1, -0.8,  0.1};

float ship1c[] = {11,   0.4, 0.2, -1,  1,    0.2, 0.9,  0.7,
                  0.6,  0.9, 0.2, 0.9, -0.2, 0.7, -0.6, 0.2,
                  -0.9, -1,  -1,  0.4, -0.2, 0.4, 0.2};
