#define MAX_CLIENTS 8
#define SIZE 1024 // dimensione del buffer
#define UDP_BUFF 50
#define UDP_PORT 8000
#define TCP_PORT 10000
#define THREAD_LIFETIME	10
#define IP_SIZE 20
#define QUEUELEN 10
#define FILENAME "ip_list.txt"
#define SEMAPH 0
#define DEBUG 0

// TCP
void TCPserver (void);
void serve_client (int csk, char clnt_ip[]);

// UDP
void thread_UDPserver (void);
void *udp_polling (void *arg);
void udp_set_non_blocking_mode (int sk);

// UTILITY
void add_device (FILE *fp, char file_name[], char ip[]);
int search_static_ip (FILE *fp, char file_name[], char clnt_ip[]);
int load_devices (FILE *fp, char file_name[], int max_devices);
void print_devices (int n_devices); 
int update_state (char ip[], char new_state[]);
void init_rand();
int rand_gen (int min, int max);
void handle_error (char message[]);
