#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>

#include <GL/glew.h>
#include <SDL.h>
#include <SDL_opengl.h>

#define MAX_PARTICLES 40768
#define SPAWN_PARTICLE_AMOUNT 128
#define SPAWN_PARTICLE_BURST_AMOUNT 512
#define PARTICLE_DEFAULT_LIFE 2000

#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720

typedef struct Particle 
{
	float 			x;
	float 			y;
	float 			dx;
	float 			dy;
	long long   	life;

} particle_t;

int main( int agrc, char **argv )
{
	// Set Random Seed
	srand( time(NULL) );

	// Initialise SDL
	if ( SDL_Init( SDL_INIT_VIDEO ) != 0 )
	{
		fprintf( stderr, "Error: Unable to Initialise SDL:\n%s\n", SDL_GetError() );
		return 1;
	}

	// Set SDL GL Compatibility
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE );
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 3 );
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 2 );

	// Create Window
	SDL_Window *window = SDL_CreateWindow( "Simple Particle System", 
											128, 128, 
											SCREEN_WIDTH, SCREEN_HEIGHT,
											SDL_WINDOW_OPENGL );
	if ( window == NULL )
	{
		fprintf( stderr, "Error: Could not create SDL Window!" );
		SDL_Quit();
		return 1;
	}

	// Create GL Context for Window
	SDL_GLContext context = SDL_GL_CreateContext( window );
	if ( context == NULL )
	{
		fprintf( stderr, "Error: Could not create SDL OpenGL Context\n" );
		SDL_DestroyWindow( window );
		SDL_Quit();
		return 1;
	}

	// Initialise GLEW
    glewExperimental = true;
    if ( glewInit() != GLEW_OK )
	{
		fprintf( stderr, "Error: Could not init GLEW Library\n" );
		SDL_GL_DeleteContext( context );
		SDL_DestroyWindow( window );
		SDL_Quit();
		return -2;
	}

    // Define Vertex Array Objects 
	GLuint vao;
	glGenVertexArrays( 1, &vao );
	glBindVertexArray( vao );

    // Create Vertex Object Buffer
    GLuint vbo;
    glGenBuffers( 1, &vbo );

    // Bind VBO
    glBindBuffer( GL_ARRAY_BUFFER, vbo );

	// Shader sources
	const GLchar *const vert_shader_code = 
	    "#version 150 core\n"
	    "in vec2 position;"
	    "void main() {"
	    "   gl_Position = vec4(position, 0.0, 1.0);"
	    "}";
	const GLchar *const frag_shader_code = 
	    "#version 150 core\n"
	    "out vec4 outColor;"
	    "void main() {"
	    "   outColor = vec4(1.0, 1.0, 1.0, 1.0);"
	    "}";

	// Define Buffer Data for Error Checking
	char buffer[512];
	GLint status;

	// Compile Shaders
	GLuint vertex_shader = glCreateShader( GL_VERTEX_SHADER );
	glShaderSource( vertex_shader, 1, &vert_shader_code, NULL );
	glCompileShader( vertex_shader );
	glGetShaderiv( vertex_shader, GL_COMPILE_STATUS, &status );
	if ( status != GL_TRUE )
	{
		glGetShaderInfoLog( vertex_shader, 512, NULL, buffer );
		fprintf( stderr, "%s", buffer );
	}
	GLuint frag_shader = glCreateShader( GL_FRAGMENT_SHADER );
	glShaderSource( frag_shader, 1, &frag_shader_code, NULL );
	glCompileShader( frag_shader );
	glGetShaderiv( frag_shader, GL_COMPILE_STATUS, &status );
	if ( status != GL_TRUE )
	{
		glGetShaderInfoLog( frag_shader, 512, NULL, buffer );
		fprintf( stderr, "%s", buffer );
	}

	// Combine Shaders into a Program
	GLuint shader_program = glCreateProgram();
	glAttachShader( shader_program, vertex_shader );
	glAttachShader( shader_program, frag_shader );
	glBindFragDataLocation( shader_program, 0, "outColor" );
	glLinkProgram( shader_program );
	glUseProgram( shader_program );

	// Link Vertex Attribs
	GLint pos_attrib = glGetAttribLocation( shader_program, "position" );
	glVertexAttribPointer( pos_attrib, 2, GL_FLOAT, GL_FALSE, 0, 0 );
	glEnableVertexAttribArray( pos_attrib );

	// Define Particle Data
	size_t particle_count = 0;
	char *memory_block = (char *) malloc( sizeof(GLfloat) * MAX_PARTICLES * 2 +
										  sizeof(particle_t) * MAX_PARTICLES );
	if ( memory_block == NULL )
	{
		fprintf( stderr, "Error: Could not allocate memory for particles: %u bytes\n",
				 (size_t)(sizeof(GLfloat) * MAX_PARTICLES) );
		SDL_GL_DeleteContext( context );
		SDL_DestroyWindow( window );
		SDL_Quit();
		return 1;
	}
	GLfloat 	*positions = (GLfloat *) memory_block;
	particle_t 	*particles = (particle_t *) ( memory_block + (sizeof(GLfloat) * MAX_PARTICLES * 2) );

	// Successful Initialisation
	fprintf( stdout, "Successful Initialisation. Now Entering Main Loop...\n" );

	// Simple Event Loop
	SDL_Event event;
	bool running = true;
	Uint32 current_time;
	Uint32 last_time = SDL_GetTicks();
	float dt;
	bool clicked_l = false;
	bool clicked_r = false;
	int mouse_x, mouse_y;
	while( running )
	{
		/* ******************** */
		/* Events 				*/
		/* ******************** */

		// Poll For Events
		while ( SDL_PollEvent( &event ) )
		{
			// Quit Program
			if ( event.type == SDL_WINDOWEVENT )
			{
				if ( event.window.event == SDL_WINDOWEVENT_CLOSE )
					running = false;
			}
		}

		// Mouse Values
		SDL_PumpEvents();
		clicked_l = SDL_GetMouseState( NULL, NULL ) & SDL_BUTTON( SDL_BUTTON_LEFT );
		clicked_r = SDL_GetMouseState( &mouse_x, &mouse_y ) & SDL_BUTTON( SDL_BUTTON_RIGHT );

		/* ******************** */
		/* Update 				*/
		/* ******************** */

		// Spawn New Particles
		if ( clicked_l )
		{
			unsigned i;
			for( i = 0; i < SPAWN_PARTICLE_AMOUNT; ++i )
			{
				if ( particle_count >= MAX_PARTICLES )
					break;

				particles[particle_count].x = (float) mouse_x;
				particles[particle_count].y = (float) mouse_y;
				particles[particle_count].dx = -200.0f + (float)(rand()%40000) / 100.0f;
				particles[particle_count].dy = -800.0f - (float)(rand()%80000) / 100.0f;
				particles[particle_count].life = 
							PARTICLE_DEFAULT_LIFE + ( rand() % PARTICLE_DEFAULT_LIFE );

				++particle_count;
			}
		}
		if ( clicked_r )
		{
			unsigned i;
			for( i = 0; i < SPAWN_PARTICLE_BURST_AMOUNT; ++i )
			{
				if ( particle_count >= MAX_PARTICLES )
					break;

				particles[particle_count].x = (float) mouse_x;
				particles[particle_count].y = (float) mouse_y;
				particles[particle_count].dx = -200.0f + (float)(rand()%40000) / 100.0f;
				particles[particle_count].dy = -600.0f + (float)(rand()%80000) / 100.0f;
				particles[particle_count].life = 
							PARTICLE_DEFAULT_LIFE + ( rand() % PARTICLE_DEFAULT_LIFE );

				++particle_count;
			}
		}

		// Find Delta Time
		current_time = SDL_GetTicks();
		dt = (float)( current_time - last_time ) / 1000.0f;
		last_time = current_time;

		// Update Particles
		for( size_t i = 0; i < particle_count;  )
		{
			const float GRAVITY = 1000.0f;

			particles[i].life -= (long long)( current_time - last_time );
			particles[i].dy += GRAVITY * dt;
			particles[i].x += particles[i].dx * dt;
			particles[i].y += particles[i].dy * dt;

			if ( particles[i].life <= 0 ||
				 particles[i].x <= 0.0f ||
				 //particles[i].y <= 0.0f ||
				 particles[i].x >= (float)SCREEN_WIDTH ||
				 particles[i].y >= (float)SCREEN_HEIGHT )
			{
				particles[i]     = particles[particle_count-1];
				positions[i*2]   = positions[ (particle_count-1) * 2 ];
				positions[i*2+1] = positions[ (particle_count-1) * 2 + 1 ];

				--particle_count;
				continue;
			}

			positions[i*2] 	 = -1.0 + (particles[i].x / (float)SCREEN_WIDTH) * 2;  // x
			positions[i*2+1] = -(-1.0 + (particles[i].y / (float)SCREEN_HEIGHT) * 2); // y

			// We Increment Here as we don't want to increase i if 
			// the particle was destroyed
			++i;
		}

		/* ******************** */
		/* Render 				*/
		/* ******************** */

		// Clear Screen
		glClearColor( 0.0f, 0.0f, 0.0f, 1.0f ); // Black
		glClear( GL_COLOR_BUFFER_BIT );

			// Drawing
			glBufferData( GL_ARRAY_BUFFER, (particle_count*2) * sizeof(GLfloat), 
						  positions, GL_STREAM_DRAW );
			glDrawArrays( GL_POINTS, 0, particle_count );

		// Swap Window Buffer
		SDL_GL_SwapWindow( window );
	}

	// Free Memory/Resources
	free( memory_block );
	SDL_GL_DeleteContext( context );
	SDL_DestroyWindow( window );
	SDL_Quit();

	// Return Success
	return 0;
}