#include <gtk/gtk.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>

// DO NOT TOUCH
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_RESET   "\x1b[0m"

#define CL_M 0
#define CL_L 1
#define CL_H 2
#define CL_Q 3

#define FORMAT_INFORMATION_SIZE 15
#define VERSION_INFORMATION_SIZE 18

// GTK PARAMETERS
#define WINDOW_SIZE 800

// QRCODE PARAMETERS
#define QRCODE_SIZE WINDOW_SIZE
#define QRCODE_VERSION 2

#define FINDER_SIZE 7
#define ALIGN_SIZE 5
#define MASK 7

#define CORRECTION_LEVEL CL_M

// GLOBAL VARIABLES
int module;
int qr_module;

int* coordinates;
int align_count;

uint16_t format_information[FORMAT_INFORMATION_SIZE];
uint32_t version_information[VERSION_INFORMATION_SIZE];

static void rgb(cairo_t *cr, int r, int g, int b) {
    float x, y, z;
    float NUM = 255.0;

    x = (float)r/NUM;
    y = (float)g/NUM;
    z = (float)b/NUM;

    cairo_set_source_rgb(cr, x, y, z);
}

static void blank(cairo_t *cr) {
    cairo_set_source_rgb(cr, 255, 255, 255);
    cairo_rectangle(cr, 0, 0, QRCODE_SIZE, QRCODE_SIZE);
    cairo_fill(cr);
}

static void make_grid(cairo_t *cr) {
    rgb(cr, 212, 226, 247);
    for (int y=0; y < qr_module*module; y = y+module) {
        for (int x=0; x < qr_module*module; x = x+module) {
            if ((x/module + y/module) % 2 == 0) {
                cairo_rectangle(cr, x, y, module, module);
            }
        }
    }
    cairo_fill(cr);
    cairo_set_source_rgb(cr, 0, 0, 0);
}

// FINDER PATTERN
static int in_finder_pattern(int qr_x, int qr_y) {
    // IN TOP LEFT
    if (qr_x <= FINDER_SIZE+1 && qr_y <= FINDER_SIZE+1) {
        return 1;
    }

    // IN TOP RIGHT
    if (qr_x >= qr_module-FINDER_SIZE-1 && qr_y <= FINDER_SIZE+1) {
        return 1;
    }

    // IN BOTTOM LEFT
    if (qr_x <= FINDER_SIZE+1 && qr_y >= qr_module-FINDER_SIZE-1) {
        return 1;
    }

    return 0;
}

static int finder_pattern(int qr_x, int qr_y) {
	//LEFT
	if (qr_x == 0 && qr_y <= FINDER_SIZE-1) {
		return 1;
	}

	//TOP
	if (qr_x <= FINDER_SIZE-1 && qr_y == 0) {
		return 1;
	}

	//RIGHT
	if (qr_x == 6 && qr_y <= FINDER_SIZE-1) {
		return 1;
	}

	//BOTTOM
	if (qr_x <= FINDER_SIZE-1 && qr_y == 6) {
		return 1;
	}

	//CENTER
	if (qr_x >= 2 && qr_x <= 4) {
		if (qr_y >= 2 && qr_y <= 4) {
			return 1;
		}
	}

	return 0;
}

static int draw_finder_pattern(int qr_x, int qr_y) {
    if (finder_pattern(qr_x, qr_y)) {
        return 1;
    }

    if (finder_pattern(qr_module-1-qr_x, qr_y)) {
	return 1;
    }

    if (finder_pattern(qr_x, qr_module-1-qr_y)) {
	return 1;
    }

    return 0;
}

// TIMING PATTERN
static int in_timing_pattern(int qr_x, int qr_y) {
    if (qr_x == FINDER_SIZE-1 || qr_y == FINDER_SIZE-1) {
        return 1;
    }
    return 0;
}

static int draw_timing_pattern(int qr_x, int qr_y) {
    if (qr_x >= qr_module || qr_y >= qr_module) { return 0; }

    if (qr_x >= FINDER_SIZE+1 && qr_x <= qr_module-FINDER_SIZE-1) {
        if (qr_y == FINDER_SIZE-1) {
            return (qr_x % 2 == 0);
	}
    }

    if (qr_y >= FINDER_SIZE+1 && qr_y <= qr_module-FINDER_SIZE-1) {
	if (qr_x == FINDER_SIZE-1) {
	    return (qr_y % 2 == 0);
	}
    }
    return 0;
}

// ALIGNMENT PATTERN
static int* get_coordinates() {
    static int coordinates[7];
    for (int i=0; i<7; i++) { coordinates[i] = 0; }

    if (QRCODE_VERSION <= 1) { return coordinates; }

    coordinates[0] = 6;

    if (QRCODE_VERSION <= 6) {
	coordinates[1] = 18 + 4*(QRCODE_VERSION - 2);
    } else if (QRCODE_VERSION <= 13) {
	coordinates[1] = 22 + 2*(QRCODE_VERSION - 7);
	coordinates[2] = 38 + 4*(QRCODE_VERSION - 7);
    } else if (QRCODE_VERSION <= 20) {
	coordinates[1] = 26 + 4*((QRCODE_VERSION-14)/3);
	coordinates[2] = 46 + 2*(QRCODE_VERSION-14) + 2*((QRCODE_VERSION-14)/3);
	coordinates[3] = 66 + 4*(QRCODE_VERSION-14);
    } else if (QRCODE_VERSION <= 27) {
	coordinates[1] = 28 + 4*(QRCODE_VERSION-21) - 6*((QRCODE_VERSION-20)/2);
	coordinates[2] = 50 + 4*((QRCODE_VERSION-21)/2);
	coordinates[3] = 72 + 2*(QRCODE_VERSION-21) + 2*((QRCODE_VERSION-21)/2);
	coordinates[4] = 94 + 4*(QRCODE_VERSION-21);
    } else if (QRCODE_VERSION <= 34) {
	coordinates[1] =  26 + 4*(QRCODE_VERSION-28) - 8 * ((QRCODE_VERSION-27)/3);
	coordinates[2] =  50 + 4*(QRCODE_VERSION-28) - 6 * ((QRCODE_VERSION-27)/3);
	coordinates[3] =  74 + 4*(QRCODE_VERSION-28) - 4 * ((QRCODE_VERSION-27)/3);
	coordinates[4] =  98 + 4*(QRCODE_VERSION-28) - 2 * ((QRCODE_VERSION-27)/3);
	coordinates[5] = 122 + 4*(QRCODE_VERSION-28);
    } else if (QRCODE_VERSION <= 40) {
	coordinates[1] =  30 + 4*(QRCODE_VERSION-35) - 10 * ((QRCODE_VERSION-33)/3);
	coordinates[2] =  54 + 4*(QRCODE_VERSION-35) -  8 * ((QRCODE_VERSION-33)/3);
	coordinates[3] =  78 + 4*(QRCODE_VERSION-35) -  6 * ((QRCODE_VERSION-33)/3);
	coordinates[4] = 102 + 4*(QRCODE_VERSION-35) -  4 * ((QRCODE_VERSION-33)/3);
	coordinates[5] = 126 + 4*(QRCODE_VERSION-35) -  2 * ((QRCODE_VERSION-33)/3);
	coordinates[6] = 150 + 4*(QRCODE_VERSION-35);
    }
    return coordinates;
}

static int verify_alignment_pattern(int qr_x, int qr_y, int pos_x, int pos_y) {
    int m = ALIGN_SIZE/2;
    
    if (qr_x >= pos_x-m-1 && qr_x <= pos_x+m-1) {
        if (qr_y >= pos_y-m-1 && qr_y <= pos_y+m-1) {
            // COLLISION FINDER PATTERN
            if (pos_x-m-1 <= FINDER_SIZE && pos_y-m-1 <= FINDER_SIZE) {
                return 0;
            }

            if (pos_x+m-1 >= qr_module-FINDER_SIZE && pos_y-m-1 <= FINDER_SIZE) {
                return 0;
            }

            if (pos_x-m-1 <= FINDER_SIZE && pos_y+m-1 >= qr_module-FINDER_SIZE) {
                return 0;
            }

            // COLLISION TIMING PATTERN
            if (pos_x >= FINDER_SIZE-m && pos_x <= FINDER_SIZE+m) {
                return 0;
            }
            
            if (pos_y >= FINDER_SIZE-m && pos_y <= FINDER_SIZE+m) {
                return 0;
            }

            return 1;
        }
    }
    return 0;
}

static int in_alignment_pattern(int qr_x, int qr_y) {
    int px, py;
    int m = ALIGN_SIZE/2;

    for (int i=0; i<7; i++) {
        px = coordinates[i];
        if (!px) continue;
        for (int j=0; j<7; j++) {
            py = coordinates[j];
            if (!py) continue;

            if (verify_alignment_pattern(qr_x, qr_y, px, py)) {
                return 1;
            }
        }
    }
    return 0;
}

static int alignment_pattern(int qr_x, int qr_y, int pos_x, int pos_y) {
    int cx = pos_x - ALIGN_SIZE/2 - 1;
    int cy = pos_y - ALIGN_SIZE/2 - 1;

    // TOP & BOTTOM
    if (qr_x >= cx && qr_x <= cx+ALIGN_SIZE-1) {
        if (qr_y == cy || qr_y == cy+ALIGN_SIZE-1) {
            return 1;
        }
    }

    // LEFT & RIGHT
    if (qr_y >= cy && qr_y <= cy+ALIGN_SIZE-1) {
        if (qr_x == cx || qr_x == cx+ALIGN_SIZE-1) {
            return 1;
        }
    }

    // CENTER
    if (qr_x == cx+ALIGN_SIZE/2 && qr_y == cy+ALIGN_SIZE/2) {
        return 1;
    }

    return 0;
}

static int draw_alignment_pattern(int qr_x, int qr_y) {
    for (int i=0; i<7; i++) {
        if (coordinates[i] == 0) { continue; }
        for (int j=0; j<7; j++) {
            if (coordinates[j] == 0) { continue; }
            
            if (verify_alignment_pattern(qr_x, qr_y, coordinates[i], coordinates[j])) {
                if (alignment_pattern(qr_x, qr_y, coordinates[i], coordinates[j])) {
                    return 1;
                }
                return 0;
            }
            
        }
    }
    return 0;
}

// DATA MASKING
static int data_masking(int qr_x, int qr_y) {
   if (in_finder_pattern(qr_x, qr_y))    { return 0; }
   if (in_timing_pattern(qr_x, qr_y))    { return 0; }
   if (in_alignment_pattern(qr_x, qr_y))    { return 0; }

   switch (MASK) {
      case 0:
          return (qr_x + qr_y) % 2 == 0;
          break;
      case 1:
          return qr_y % 2 == 0;
          break;
      case 2:
          return qr_x % 3 == 0;
          break;
      case 3:
          return (qr_x + qr_y) % 3 == 0;
          break;
      case 4:
          return (qr_y/2 + qr_x/3) % 2 == 0;
          break;
      case 5:
          return ((qr_x * qr_y) % 2 + (qr_x * qr_y) % 3) == 0;
          break;
      case 6:
          return ((qr_x * qr_y) % 2 + (qr_x * qr_y) % 3) % 2 == 0;
          break;
      case 7:
          return ((qr_x + qr_y) % 2 + (qr_x * qr_y) % 3) % 2 == 0;
          break;
      default:
          return 1;
          break;
   }
}

static int
draw_data_masking(int qr_x, int qr_y) {
    return data_masking(qr_x, qr_y);
}

// FORMAT INFORMATION
static uint16_t get_format() {
    switch(CORRECTION_LEVEL) {
        case CL_M:
            switch(MASK) {
                case 0:  return 0b101010000010010;
                case 1:  return 0b101000100100101;
                case 2:  return 0b101111001111100;
                case 3:  return 0b101101101001011;
                case 4:  return 0b100010111111001;
                case 5:  return 0b100000011001110;
                case 6:  return 0b100111110010111;
                case 7:  return 0b100101010100000; 
                default: return 0b000000000000000;
            }
            break;
        
        case CL_L:
            switch(MASK) {
                case 0:  return 0b111011111000100;
                case 1:  return 0b111001011110011;
                case 2:  return 0b111110110101010;
                case 3:  return 0b111100010011101;
                case 4:  return 0b110011000101111;
                case 5:  return 0b110001100011000;
                case 6:  return 0b110110001000001;
                case 7:  return 0b110100101110110; 
                default: return 0b000000000000000;
            }
            break;

        case CL_H:
            switch(MASK) {
                case 0:  return 0b001011010001001;
                case 1:  return 0b001001110111110;
                case 2:  return 0b001110011100111;
                case 3:  return 0b001100111010000;
                case 4:  return 0b000011101100010;
                case 5:  return 0b000001001010101;
                case 6:  return 0b000110100001100;
                case 7:  return 0b000100000111011; 
                default: return 0b000000000000000;
            }
            break;

        case CL_Q:
            switch(MASK) {
                case 0:  return 0b011010101011111;
                case 1:  return 0b011000001101000;
                case 2:  return 0b011111100110001;
                case 3:  return 0b011101000000110;
                case 4:  return 0b010010010110100;
                default: return 0b000000000000000;
            }
            break;

        default:
            return 0b000000000000000;
    }
}

static void store_format_information() {
    uint16_t format = get_format();
    
    for (int i=0; i<FORMAT_INFORMATION_SIZE; i++) {
        format_information[i] = (format >> i) & 1;
    }
}

static int draw_format_information(int qr_x, int qr_y) {
    // TOP LEFT FORMAT INFORMATION
    if (qr_x == FINDER_SIZE+1 && qr_y <= FINDER_SIZE+1) {
        if (qr_y < FINDER_SIZE-1) {
            return format_information[qr_y];
        }
        if (qr_y > FINDER_SIZE-1) {
            return format_information[qr_y-1];
        }
    }

    if (qr_x < FINDER_SIZE+1 && qr_y == FINDER_SIZE+1) {
        if (qr_x > FINDER_SIZE-1) {
            return format_information[FORMAT_INFORMATION_SIZE-qr_x];
        }

        if (qr_x < FINDER_SIZE-1) {
            return format_information[FORMAT_INFORMATION_SIZE-qr_x-1];
        }
    }

    // TOP RIGHT FORMAT INFORMATION
    if (qr_x >= qr_module-FINDER_SIZE-1 && qr_y == FINDER_SIZE+1) {
        return format_information[qr_module-qr_x-1];
    }

    // BOTTOM LEFT FORMAT INFORMATION
    if (qr_x == FINDER_SIZE+1 && qr_y >= qr_module-FINDER_SIZE) {
        return format_information[FORMAT_INFORMATION_SIZE-qr_module+qr_y];
    }

    return 0;
} 

// VERSION INFORMATION
static uint32_t get_version_information() {
    switch (QRCODE_VERSION) {
        case 7 : return 0b000111110010010100;
        case 8 : return 0b001000010110111100;
        case 9 : return 0b001001101010011001;
        case 10: return 0b001010010011010011; 
        case 11: return 0b001011101111110110;
        case 12: return 0b001100011101100010; 
        case 13: return 0b001101100001000111; 
        case 14: return 0b001110011000001101;
        case 15: return 0b001111100100101000;
        case 16: return 0b010000101101111000; 
        case 17: return 0b010001010001011101;
        case 18: return 0b010010101000010111; 
        case 19: return 0b010011010100110010;
        case 20: return 0b010100100110100110; 
        case 21: return 0b010101011010000011; 
        case 22: return 0b010110100011001001; 
        case 23: return 0b010111011111101100; 
        case 24: return 0b011000111011000100; 
        case 25: return 0b011001000111100001; 
        case 26: return 0b011010111110101011; 
        case 27: return 0b011011000010001110; 
        case 28: return 0b011100110000011010; 
        case 29: return 0b011101001100111111; 
        case 30: return 0b011110110101110101; 
        case 31: return 0b011111001001010000; 
        case 32: return 0b100000100111010101;
        case 33: return 0b100001011011110000;
        case 34: return 0b100010100010111010; 
        case 35: return 0b100011011110011111;
        case 36: return 0b100100101100001011; 
        case 37: return 0b100101010000101110; 
        case 38: return 0b100110101001100100; 
        case 39: return 0b100111010101000001;
        case 40: return 0b101000110001101001; 
        default: return 0b000111110010010100;
    }
}

static void store_version_information() {
    uint16_t version = get_version_information();
    
    for (int i=0; i<VERSION_INFORMATION_SIZE; i++) {
        version_information[i] = (version >> i) & 1;
    }
}

static int draw_version_information(int qr_x, int qr_y) {
    if (QRCODE_VERSION < 7) {
        return 0;
    }

    uint32_t version = get_version_information();

    int x, y;

    // BOTTOM LEFT VERSION INFORMATION
    if (qr_x < FINDER_SIZE-1) {
        if (qr_y >= qr_module-FINDER_SIZE-2-2 && qr_y < qr_module-FINDER_SIZE-1) {
            x = qr_x;
            y = qr_y-(qr_module-FINDER_SIZE-2-2);

            return version_information[3*x+y];
        }
    }
    
    // TOP RIGHT VERSION INFORMATION
    if (qr_y < FINDER_SIZE-1) {
        if (qr_x >= qr_module-FINDER_SIZE-2-2 && qr_x < qr_module-FINDER_SIZE-1) {
            x = qr_x-(qr_module-FINDER_SIZE-2-2);
            y = qr_y;
            
            return version_information[3*y+x];
        }
    }
    return 0;
}

// DARK MODULE
static int draw_dark_module(int qr_x, int qr_y) {
    if (qr_x == FINDER_SIZE+1 && qr_y == qr_module-FINDER_SIZE-1) {
        return 1;
    }
    return 0;
}

static void
draw_qrcode(cairo_t *cr) 
{
    int x = 0;
    int y = 0;

    int qr_x = 0;
    int qr_y = 0;

    int black = 0;
    
    for (y = 0; y <= QRCODE_SIZE - module; y = y+module) {
        for (x = 0; x <= QRCODE_SIZE - module; x = x + module) {
            qr_x = x / module;
            qr_y = y / module;
        
        // FULL QR_CODE
	    // black = draw_finder_pattern(qr_x, qr_y);
	    // black = black || draw_timing_pattern(qr_x, qr_y);
        // black = black || draw_alignment_pattern(qr_x, qr_y);
        // black = black || draw_data_masking(qr_x, qr_y);
        // black = black || draw_format_information(qr_x, qr_y);
        // black = black || draw_dark_module(qr_x, qr_y);
        // black = black || draw_version_information(qr_x, qr_y);
        
        // DEBUG
	    // black = draw_finder_pattern(qr_x, qr_y);
	    // black = draw_timing_pattern(qr_x, qr_y);
        // black = draw_alignment_pattern(qr_x, qr_y);
        // black = draw_data_masking(qr_x, qr_y);
        // black = draw_format_information(qr_x, qr_y);
        // black = draw_dark_module(qr_x, qr_y);
        // black = draw_version_information(qr_x, qr_y);

        black = black & !(qr_x >= qr_module || qr_y >= qr_module);
        if (black) {
	        cairo_rectangle(cr, x, y, module, module);
	    }
	}
    }
}

static void
draw_cb (GtkDrawingArea *area,
         cairo_t        *cr,
         gpointer        data)
{
    blank(cr);
    make_grid(cr);
    cairo_set_source_rgb(cr, 0, 0, 0);
    draw_qrcode(cr);
    cairo_fill(cr);
}

static void
activate (GtkApplication *app, gpointer user_data)
{
    GtkWidget *window;
    GtkWidget *drawing_area;

    window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Carré GTK");
    gtk_window_set_default_size(GTK_WINDOW(window), WINDOW_SIZE, WINDOW_SIZE);

    drawing_area = gtk_drawing_area_new();
    gtk_drawing_area_set_draw_func(
        GTK_DRAWING_AREA(drawing_area),
        draw_cb,
        NULL,
        NULL
    );

    gtk_window_set_child(GTK_WINDOW(window), drawing_area);
    gtk_window_present(GTK_WINDOW(window));
}

static void handle_errors() {
    if (MASK >= 8) {
        printf(ANSI_COLOR_RED "Incorrect value of MASK=%d" ANSI_COLOR_RESET "\n", MASK);
        exit(0);
    }

    if (CORRECTION_LEVEL >= 4) {
        printf(ANSI_COLOR_RED "Incorrect CORRECTION_LEVEL=%d" ANSI_COLOR_RESET "\n", CORRECTION_LEVEL);
        exit(0);
    }
    
    uint16_t format = get_format();
    if (format == 0) {
        printf(ANSI_COLOR_RED "CORRECTION_LEVEL=%d too high." ANSI_COLOR_RESET "\n", CORRECTION_LEVEL);
        exit(0);
    }

    if (QRCODE_VERSION <= 0 || QRCODE_VERSION > 40) {
        printf(ANSI_COLOR_RED "Incorrect value of VERSION=%d" ANSI_COLOR_RESET "\n", QRCODE_VERSION);
        exit(0);
    }
}

static void setup() {
    qr_module = 17 + 4*QRCODE_VERSION;
    module = floor(QRCODE_SIZE/qr_module);
    coordinates = get_coordinates();
    store_format_information(); // Stored in format_information
    
    if (QRCODE_VERSION > 0 && QRCODE_VERSION <= 40)
        store_version_information();

    // CHECK IF IT CAN PRINT THE QR CODE
    handle_errors();
}

int main (int argc, char **argv)
{
    setup();

    GtkApplication *app;
    int status;

    app = gtk_application_new(NULL, G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return status;
}
