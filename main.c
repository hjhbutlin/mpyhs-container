// Brown-Butlin Placeholder

// ========================= CONFIG =========================

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <math.h>
#define GL_SILENCE_DEPRECATION
#include <GLUT/glut.h>

#define WINDOW_NAME "eggs"
#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 800
const float BACKGROUND_COLOUR[4] = {1.0f, 1.0f, 1.0f, 1.0f};
const float BUTTON_COLOURS[9] = {0.0f, 0.0f, 0.0f, // chilling
                                 0.5f, 0.5f, 0.5f, // hovered
                                 0.8f, 0.8f, 0.8f}; // clicked

#define RENDER_MPF 20 // milliseconds per frame; 20 => 50fps
#define BUTTON_COUNT 2  // Updated to include back button

// loading stuff
#define CIRCLE_RADIUS 10.0f
#define CIRCLE_SPACING 30.0f
#define CIRCLE_BASE_Y 0.0f  // centered vertically
#define HOP_HEIGHT 15.0f
#define HOP_DURATION 300 // milliseconds
#define CIRCLE_COUNT 3

bool simulationRunning = false;
pthread_t simulationThread;
pthread_mutex_t stateMutex = PTHREAD_MUTEX_INITIALIZER;

// window input
#define ROTATION_SCALE 0.6f
float rotationX = 0.0f;
float rotationY = 0.0f;
int lastMouseX = 0;
int lastMouseY = 0;
bool mouseDown = false;

typedef struct {
    bool visible;
    float x, y, width, height;
    bool hovered;
    bool clicked;
    void (*onClick)(); // function pointer for click action
    const char* text;
} Button;

typedef struct {
    float x, y;
    int hopState; // 0: resting, 1: going up, 2: going down
    unsigned long hopStartTime;
} LoadingCircle;

LoadingCircle loadingCircles[CIRCLE_COUNT];
unsigned long currentTime = 0; // animation time tracking

// forward declarations for button click handlers
void startButtonClicked();
void backButtonClicked();

Button buttons[BUTTON_COUNT] = {
    // visible, X, Y, width, height, hovered, clicked, function, text
    {true, -40, -20, 80, 40, false, false, NULL, "START"}, // START BUTTON
    {false, -380, 350, 80, 40, false, false, NULL, "BACK"}  // BACK BUTTON
};

// ==========================================================






// ==================== SIMULATION CODE =====================

void initLoadingAnimation() {
    float startX = -CIRCLE_SPACING;
    for (int i = 0; i < CIRCLE_COUNT; i++) {
        loadingCircles[i].x = startX + (i * CIRCLE_SPACING);
        loadingCircles[i].y = CIRCLE_BASE_Y;
        loadingCircles[i].hopState = 0;
        loadingCircles[i].hopStartTime = 0;
    }
}

void* runSimulation(void* arg) {
    (void)arg;
    
    printf("sim thread started\n");
    
    while (simulationRunning) {
        // sim is all in da rendering
        usleep(100000); // 100ms sleep
    }
    
    printf("sim thread ended.\n");
    pthread_exit(NULL);
    return NULL;
}

void updateLoadingAnimation() {
    currentTime += RENDER_MPF;
    
    int activeCircle = (currentTime / HOP_DURATION) % CIRCLE_COUNT;
    
    for (int i = 0; i < CIRCLE_COUNT; i++) {
        if (i == activeCircle) {
            unsigned long circleTime = currentTime % (HOP_DURATION);
            
            // going up
            if (circleTime < HOP_DURATION / 2) {
                float progress = (float)circleTime / (HOP_DURATION / 2);
                loadingCircles[i].y = CIRCLE_BASE_Y + (HOP_HEIGHT * progress);
                loadingCircles[i].hopState = 1;
            } 
            // going down
            else {
                float progress = (float)(circleTime - HOP_DURATION / 2) / (HOP_DURATION / 2);
                loadingCircles[i].y = CIRCLE_BASE_Y + HOP_HEIGHT - (HOP_HEIGHT * progress);
                loadingCircles[i].hopState = 2;
            }
        } else {
            // chillin
            loadingCircles[i].y = CIRCLE_BASE_Y;
            loadingCircles[i].hopState = 0;
        }
    }
}

void startSimulation() {
    pthread_mutex_lock(&stateMutex);
    if (!simulationRunning) {
        simulationRunning = true;
        int result = pthread_create(&simulationThread, NULL, runSimulation, NULL);
        if (result != 0) {
            printf("Failed to create simulation thread: %d\n", result);
            simulationRunning = false;
        } else {
            // back button visible
            buttons[1].visible = true;
        }
    }
    pthread_mutex_unlock(&stateMutex);
}

void stopSimulation() {
    pthread_mutex_lock(&stateMutex);
    if (simulationRunning) {
        simulationRunning = false;
        pthread_join(simulationThread, NULL);
    }
    pthread_mutex_unlock(&stateMutex);
}

// ==========================================================






// ======================= UTILITIES ========================

bool isPointInButton(Button *btn, int x, int y) {
    if (!btn->visible) return false; // invisible buttons are unclickable
    
    // convert GLUT window coordinates to OpenGL coordinate system
    float glX = x - (WINDOW_WIDTH / 2);
    float glY = (WINDOW_HEIGHT / 2) - y; // invert Y coordinate

    return (glX >= btn->x && glX <= btn->x + btn->width &&
            glY >= btn->y && glY <= btn->y + btn->height);
}

void drawText(Button* btn, float x, float y) {
    if (btn->clicked) {
        glColor3f(BUTTON_COLOURS[6],BUTTON_COLOURS[7],BUTTON_COLOURS[8]);
    } else if (btn->hovered) {
        glColor3f(BUTTON_COLOURS[3],BUTTON_COLOURS[4],BUTTON_COLOURS[5]);
    } else {
        glColor3f(BUTTON_COLOURS[0],BUTTON_COLOURS[1],BUTTON_COLOURS[2]);
    }
    glRasterPos2f(x, y);
    for (const char* c = btn->text; *c != '\0'; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
    }
}

void drawButton(Button *btn) {
    if (!btn->visible) return; // invisible buttons are invisible
    
    // button border colour
    if (btn->clicked) {
        glColor3f(BUTTON_COLOURS[6],BUTTON_COLOURS[7],BUTTON_COLOURS[8]);
    } else if (btn->hovered) {
        glColor3f(BUTTON_COLOURS[3],BUTTON_COLOURS[4],BUTTON_COLOURS[5]);
    } else {
        glColor3f(BUTTON_COLOURS[0],BUTTON_COLOURS[1],BUTTON_COLOURS[2]);
    }
    
    // draw button outline
    glLineWidth(2.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(btn->x, btn->y);
    glVertex2f(btn->x + btn->width, btn->y);
    glVertex2f(btn->x + btn->width, btn->y + btn->height);
    glVertex2f(btn->x, btn->y + btn->height);
    glEnd();
    
    // draw button text centered
    float textWidth = glutBitmapLength(GLUT_BITMAP_HELVETICA_18, (const unsigned char*)btn->text);
    float textX = btn->x + (btn->width - textWidth) / 2;
    float textY = btn->y + (btn->height - 18) / 2;
    
    drawText(btn, textX, textY);
}

// draw the loading animation circles
void drawLoadingAnimation() {
    glPushMatrix();
    
    for (int i = 0; i < CIRCLE_COUNT; i++) {
        // Set color based on state
        if (loadingCircles[i].hopState > 0) {
            glColor3f(0.0f, 0.5f, 0.8f); // active circle is blue
        } else {
            glColor3f(0.5f, 0.5f, 0.5f); // inactive circles are gray
        }
        
        // draw the circle
        glBegin(GL_POLYGON);
        for (int j = 0; j < 20; j++) {
            float angle = 2.0f * 3.14159f * j / 20;
            float x = loadingCircles[i].x + sin(angle) * CIRCLE_RADIUS;
            float y = loadingCircles[i].y + cos(angle) * CIRCLE_RADIUS;
            glVertex2f(x, y);
        }
        glEnd();
    }
    
    glPopMatrix();
}

// ===========================================================






// ===================== RENDER PIPELINE =====================

void initOpenGL() {
    float r = BACKGROUND_COLOUR[0]; 
    float g = BACKGROUND_COLOUR[1]; 
    float b = BACKGROUND_COLOUR[2]; 
    float a = BACKGROUND_COLOUR[3];
    
    glClearColor(r, g, b, a);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-WINDOW_WIDTH / 2, WINDOW_WIDTH / 2, -WINDOW_HEIGHT / 2, WINDOW_HEIGHT / 2, -1.0f, 1.0f);
    glMatrixMode(GL_MODELVIEW);
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT);
    
    pthread_mutex_lock(&stateMutex);
    
    // Always draw visible buttons
    for (int i = 0; i < BUTTON_COUNT; i++) {
        drawButton(&buttons[i]);
    }
    
    // Draw the animation only when running
    if (simulationRunning) {
        drawLoadingAnimation();
    }
    
    pthread_mutex_unlock(&stateMutex);
    
    glFlush();
    glutSwapBuffers();
}

void update(int n) {
    (void)n;
    
    // Update loading animation if running
    pthread_mutex_lock(&stateMutex);
    if (simulationRunning) {
        updateLoadingAnimation();
    }
    pthread_mutex_unlock(&stateMutex);
    
    glutPostRedisplay();
    glutTimerFunc(RENDER_MPF, update, 0);
}

// ===========================================================






// ====================== WINDOW INPUT =======================

void mouseMove(int x, int y) {
    if (mouseDown) {
        float dx = (x - lastMouseX) * ROTATION_SCALE;
        float dy = (y - lastMouseY) * ROTATION_SCALE;
        rotationY += dx;
        rotationX += dy;
        lastMouseX = x;
        lastMouseY = y;
    }
    
    // hover detection
    for (int i = 0; i < BUTTON_COUNT; i++) {
        buttons[i].hovered = isPointInButton(&buttons[i], x, y);
    }
}

void mousePress(int button, int mouseState, int x, int y) {
    if (button == GLUT_LEFT_BUTTON) {
        if (mouseState == GLUT_DOWN) {
            mouseDown = true;
            lastMouseX = x;
            lastMouseY = y;
            
            // Click detection
            for (int i = 0; i < BUTTON_COUNT; i++) {
                if (isPointInButton(&buttons[i], x, y)) {
                    buttons[i].clicked = true;
                    if (buttons[i].onClick) {
                        buttons[i].onClick(); // call click handler if set
                    }
                } else {
                    buttons[i].clicked = false;
                }
            }
        } else if (mouseState == GLUT_UP) {
            mouseDown = false;
            
            // Reset click state
            for (int i = 0; i < BUTTON_COUNT; i++) {
                buttons[i].clicked = false;
            }
        }
    }
}

void startButtonClicked() {
    // make start button invisible and unclickable
    buttons[0].visible = false;
    
    startSimulation();
}

void backButtonClicked() {
    // Stop the simulation
    stopSimulation();
    
    // Reset UI to initial state
    buttons[0].visible = true;  // Show start button
    buttons[1].visible = false; // Hide back button
}

void cleanup() {
    stopSimulation();
    pthread_mutex_destroy(&stateMutex);
}

// ===========================================================






// ========================= MAIN ============================

int main(int argc, char** argv) {

    pthread_mutex_init(&stateMutex, NULL);
    
    atexit(cleanup);
    
    // Initialize the loading animation
    initLoadingAnimation();
    
    // add functions to buttons
    buttons[0].onClick = startButtonClicked;
    buttons[1].onClick = backButtonClicked;
    
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
    glutCreateWindow(WINDOW_NAME);
    
    glutMouseFunc(mousePress);
    glutMotionFunc(mouseMove);
    glutPassiveMotionFunc(mouseMove); // passive motion to detect hover without clicking
    
    initOpenGL();
    glutTimerFunc(RENDER_MPF, update, 0);
    glutDisplayFunc(display);
    
    glutMainLoop();
    return 0;
}
