#include "websockets.h"
#include <signal.h>
#include <math.h>

int sendCommand(struct lws *wsi,unsigned char *buf,unsigned int len);
int monID;
int spawn_id;

typedef enum DOG
{
  INIT,
  SEARCH,
  GIVE,
  DIR,
} DOG;

DOG blue = INIT;

typedef struct vecteur{
  int x;
  int y;
}vecteur;

vecteur spawnpoints[2];
vecteur direction;

typedef struct Cell
{
    unsigned int nodeID;
    unsigned int x, y;
    unsigned short size;
    unsigned char flag;
    unsigned char r,g,b;
    char* name;
}Cell;

typedef struct Pile{
  Cell *cell;
  struct Pile *next;
} Pile;

Pile* chaine = NULL;




void sighandler(int sig)
{
  forceExit = 1;
}

t_packet *allocatePacket()
{
  t_packet *tmp;
  if ((tmp = malloc(sizeof(t_packet))) == NULL) return NULL;
  memset(tmp, 0, sizeof(t_packet));
  return tmp;
}

void move(struct lws *wsi, vecteur vect)
{
  unsigned char paquet[13] = {0};
  paquet[0] = 0x10;
  memcpy(paquet+1, &(vect.x), 4);
  memcpy(paquet+5, &(vect.y), 4);
  sendCommand(wsi, paquet, 13);
}

int sendCommand(struct lws *wsi,unsigned char *buf,unsigned int len)
{
	t_packet *tmp,*list=packetList;

	if (len > MAXLEN ) return -1;
	if ((tmp=allocatePacket()) == NULL ) return -1;
	memcpy(&(tmp->buf)[LWS_PRE],buf,len);
	tmp->len=len;
	if (packetList == NULL )
		packetList=tmp;
	else {
		while (list && list->next) {
			list=list->next;
		}
		list->next=tmp;
	}
	lws_callback_on_writable(wsi);
	return 1;
}

int writePacket(struct lws *wsi)
{
	t_packet *tmp=packetList;
	int ret;

	if (packetList == NULL ) return 0;

	packetList=tmp->next;
	ret=lws_write(wsi,&(tmp->buf)[LWS_PRE],tmp->len,LWS_WRITE_BINARY);
	free(tmp);
	lws_callback_on_writable(wsi);
	return(ret);
}

void insertion(Pile **pile, Cell *upcell){

    Pile *tmp = *pile;

  	tmp=malloc(sizeof(Pile));
    tmp->next = *pile;
    tmp->cell = upcell;
    *pile = tmp;

}

void supressall(Pile **pile)
{
	Pile* tmp = *pile;
	while(*pile != NULL)
	{
		tmp = *pile;
		*pile = tmp->next;
		free(tmp->cell->name);
		free(tmp->cell);
		free(tmp);
	}
}


int j = 0;
void initialize(struct lws *wsi, Pile* pile)
{
	printf("Hey init\n");
	int calcul1, calcul2;
	Pile *tmp = pile;
	while(tmp != NULL && monID != tmp->cell->nodeID)
	{
		printf("ID :: %d // %d\n", monID, tmp->cell->nodeID);
		tmp = tmp->next;
	}
	

	if(tmp == NULL )
		return;

	if(j == 0)
	{
		if(tmp->cell->x <= 4500)
			spawn_id = 0;
		else
			spawn_id = 1;
		j = 1;
	}
	printf("Cazou\n");
	move(wsi, spawnpoints[spawn_id]);

	//if(tmp->cell->x == spawnpoints[spawn_id].x && tmp->cell->y == spawnpoints[spawn_id].y)
	printf("CELLULE : %d, %d \n", tmp->cell->x, tmp->cell->y);
	if((tmp->cell->x == 1000 && tmp->cell->y == 1000) || (tmp->cell->x == 8000 && tmp->cell->y == 5000))
		blue = SEARCH;
}

void update(unsigned char *paquet)
{
	supressall(&chaine);
	int i;
	unsigned short deadcellslength;
		memcpy(&deadcellslength, paquet+1, sizeof(unsigned short));
		i = deadcellslength*8 + 3;
		while(paquet[i] != 0)
		{
			Cell *node = malloc(sizeof(Cell));
			memcpy(node, paquet+i, 18); // 18 = sizeof(Cell) - sizeof(char*)
			i+=18;

			if (node->flag & 0x8)
			{
				node->name = malloc(strlen(paquet+i)+1);
				strcpy(node->name, paquet+i);
				i += strlen(node->name)+1;
			}
			printf("Contenu de la cellule : ID : %d x : %d y : %d \n",node->nodeID, node->x, node->y );

			insertion(&chaine, node);

		}
		i++;
		unsigned short removecellslength;
		memcpy(&removecellslength, paquet+i, sizeof(unsigned short) );
		unsigned int removecellsID;
		i+=2;

		//printNodeStack(chaine);
}

void parcours(struct lws *wsi, Pile* chaine)
{
	static int mode;
	static int etape = 0;
	Pile* tmp = chaine;
	vecteur destination;
	while(tmp->cell->nodeID != monID)
		tmp = tmp->next;

	if(tmp->cell->x == 1000 && tmp->cell->y == 1000)
		mode = 1;

	else if(tmp->cell->x == 8000 && tmp->cell->y == 5000)
		mode = 2;

	if(etape == 0)
	{
		if(mode == 1)
		{
			destination.x = 8000;
			destination.y = 1000;
		}
		else if(mode == 2)
		{
			destination.x = 1000;
			destination.y = 5000;
		}
		move(wsi, destination);
		if((tmp->cell->x == 8000 && mode == 1) || (tmp->cell->x == 1000 && mode == 2))
			etape = 1;

	}
	else if(etape == 1)
	{
		if(mode == 1)
		{
			destination.x = 8000;
			destination.y = 3000;
		}
		else if(mode == 2)
		{
			destination.x = 1000;
			destination.y = 3000;
		}
		move(wsi, destination);
		if(tmp->cell->y == 3000)
			etape = 2;
	}
	else if(etape == 2)
	{
		if(mode == 1)
		{
			destination.x = 1000;
			destination.y = 3000;
		}
		else if(mode == 2)
		{
			destination.x = 8000;
			destination.y = 3000; 
		}
		move(wsi, destination);
		if((tmp->cell->x == 1000 && mode == 1) || (tmp->cell->x == 8000 && mode == 2))
			etape = 3;
	}
	else if(etape == 3)
	{
		if(mode == 1)
		{
			destination.x = 1000;
			destination.y = 5000;
		}
		else if (mode == 2)
		{
			destination.x = 8000;
			destination.y = 1000;
		}
		move(wsi, destination);
		if((tmp->cell->y == 5000 && mode == 1) || (tmp->cell->y == 1000 && mode == 2))
			etape = 4;
	}
	else if(etape == 4)
	{
		if(mode == 1)
		{
			destination.x = 8000;
			destination.y = 5000;
		}
		else if (mode == 2)
		{
			destination.x = 1000;
			destination.y = 1000;
		}
		move(wsi, destination);
		if((tmp->cell->x == 8000 && mode == 1) || (tmp->cell->x == 1000 && mode == 2))
			etape = 0;
	}

}

/*void give(struct lws *wsi, Pile* chaine)
{
	vecteur* position;
	vecteur direction;
	vecteur delta;
	position[0].x = 3000;
	position[0].y = 2000;
	position[1].x = 6000;
	position[1].y = 2000;
	position[2].x = 6000;
	position[2].y = 4000;
	position[3].x = 3000;
	position[3].y = 4000;
	Pile* tmp = chaine;
	int i;
	while(monID != tmp->cell->nodeID)
		tmp = tmp->next;
	if( tmp->cell->x <= 4500 )
	{
		if(tmp->cell->y <= 3000)
			i = 0;
		else
			i = 3;
	}
	else
	{
		if(tmp->cell->y <= 3000)
			i = 1;
		else
			i = 2;
	}
	delta.x = (tmp->cell->x - position[i].x)/100;
	delta.y = (tmp->cell->y - position[i].y)/100;
	direction.x = tmp->cell->x + delta.x;
	direction.y = tmp->cell->x + delta.y;
	move(wsi, direction);
}*/

vecteur bot_pos;

void detect(struct lws *wsi, Pile *chaine) 
{ 
	Pile* tmp = chaine; 
	vecteur bot_pos; 
	while(tmp != NULL && strncmp("bot", tmp->cell->name, 3) != 0) 
		tmp = tmp->next; 
	if (tmp == NULL) 
		return; 
	bot_pos.x = tmp->cell->x; 
	bot_pos.y = tmp->cell->y; 
	blue = GIVE; 
 
} 
 
int k = 0;
void give(struct lws *wsi, Pile *chaine)
{
	static int compteur = 0;
	vecteur* jaune_pos;
	Pile* tmp = chaine;
	jaune_pos[0].x = 3000; jaune_pos[0].y = 2000;
	jaune_pos[1].x = 6000; jaune_pos[1].y = 2000;
	jaune_pos[2].x = 3000; jaune_pos[2].y = 4000;
	jaune_pos[3].x = 6000; jaune_pos[3].y = 4000;
	while(tmp != NULL)
		tmp = tmp->next;

	if(tmp == NULL)
		return;


	if(k == 0)
	{
		if(tmp->cell->x <= 4500 && tmp->cell->y <= 3000)
	      spawn_id = 0;
	    else if(tmp->cell->x > 4500 && tmp->cell->y <= 3000)
	      spawn_id = 1;
	    else if(tmp->cell->x > 4500 && tmp->cell->y <= 3000)
	      spawn_id = 2;
	    else if(tmp->cell->x > 4500 && tmp->cell->y > 3000)
	      spawn_id = 3;
	  	k = 1;
	}
  	move(wsi, jaune_pos[spawn_id]);

  	if(tmp->cell->x == jaune_pos[spawn_id].x && tmp->cell->y == jaune_pos[spawn_id].y)
  	{
  		compteur ++;
  		if(compteur == 30)
  		{
  			blue = DIR;
  			compteur = 0;
  		}
  	}
  	else
  		compteur = 0;

}


int first_ID = 0;
int recv_packet(unsigned char *paquet, struct lws *wsi)
{
	switch (paquet[0])
	{
		case 0x10 :
			if(blue == INIT)
			{
				printf("mode initial\n");
				update(paquet);
				Pile *tmp = chaine;

				while(tmp ->next != NULL)
					tmp = tmp->next;

				if(first_ID == 0)
				{
					monID = tmp->cell->nodeID;
					first_ID = 1;
				}
				tmp = chaine;
				while(tmp != NULL)
				{
					if(strcmp(tmp->cell->name, "blue") == 0 && (tmp->cell->nodeID != monID) )
					{
						if( (tmp->cell->x == spawnpoints[spawn_id].x && tmp->cell->y == spawnpoints[spawn_id].y))
						{
							spawn_id++;
							spawn_id = spawn_id % 2;
						}
					}
					tmp = tmp->next;
				}
				printf("\n RIP \n");
				initialize(wsi, chaine);

			}
			else if (blue == SEARCH)
			{
				printf("mode search\n");
				update(paquet);
				parcours(wsi, chaine);
				detect(wsi, chaine);
			}
			else if (blue == GIVE)
			{
				update(paquet);
				give(wsi, chaine);
				printf("BLUE GIVE\n");
			}
			else if (blue == DIR)
			{
				printf("mode DIR\n");
				update(paquet);
			}
	}
}






static int callbackOgar(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len)
{
	static unsigned int offset=0;
	static unsigned char rbuf[MAXLEN];

	unsigned char connection[5] = {0xff, 0, 0, 0, 0};
	unsigned char start[5] = {0xfe, 0x0d, 0, 0, 0};

	switch (reason) {
	case LWS_CALLBACK_CLIENT_ESTABLISHED:

		sendCommand(wsi, connection, 5);
		sendCommand(wsi, start, 5);

    char* nom = "blue";
    unsigned char* paquet = calloc(strlen(nom)+2, 1);
    memcpy(paquet+1, nom, strlen(nom));
    sendCommand(wsi, paquet, strlen(nom)+2);

		fprintf(stderr, "ogar: LWS_CALLBACK_CLIENT_ESTABLISHED\n");
		break;

 	case LWS_CALLBACK_CLIENT_WRITEABLE:
		if (writePacket(wsi) < 0 ) forceExit=1;
		break;

	case LWS_CALLBACK_CLIENT_RECEIVE:
		// we have receive some data, check if it can be written in static allocated buffer (length)

		if (offset + len < MAXLEN ) {
			memcpy(rbuf+offset,in,len);
			offset+=len;
			// we have receive some data, check with websocket API if this is a final fragment
			if (lws_is_final_fragment(wsi)) {
				// call recv function here
				recv_packet(rbuf, wsi);

				offset=0;
			}
		} else {	// length is too long... get others but ignore them...
			offset=MAXLEN;
		 	if ( lws_is_final_fragment(wsi) ) {
				offset=0;
			}
		}
		break;
	case LWS_CALLBACK_CLOSED:
		lwsl_notice("ogar: LWS_CALLBACK_CLOSED\n");
		forceExit = 1;
		break;
	case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
		lwsl_err("ogar: LWS_CALLBACK_CLIENT_CONNECTION_ERROR\n");
		forceExit = 1;
		break;

	case LWS_CALLBACK_COMPLETED_CLIENT_HTTP:
		lwsl_err("ogar: LWS_CALLBACK_COMPLETED_CLIENT_HTTP\n");
		forceExit = 1;
		break;

	default:
		break;
	}

	return 0;
}

int main(int argc, char const *argv[])
{
	if(argc < 3)
	{
		printf("Use ./prog ip port\n");
		return 1;
	}
	vecteur rdv1; rdv1.x = 1000; rdv1.y = 1000;
	vecteur rdv2; rdv2.x = 8000; rdv2.y = 5000;

	spawnpoints[0] = rdv1;
	spawnpoints[1] = rdv2;

	struct lws_context_creation_info info;
	struct lws_client_connect_info i;

	struct lws_context *context;
	const char *protocol,*temp;

	memset(&info, 0, sizeof info);
	memset(&i, 0, sizeof(i));

	signal(SIGINT, sighandler);

	i.origin = "agar.io";
	i.port = atoi(argv[2]);

	if (lws_parse_uri(argv[1], &protocol, &i.address, &i.port, &temp))
		;

	if (!strcmp(protocol, "http") || !strcmp(protocol, "ws"))
		i.ssl_connection = 0;
	if (!strcmp(protocol, "https") || !strcmp(protocol, "wss"))
		i.ssl_connection = 1;

	i.host = i.address;
	i.ietf_version_or_minus_one = -1;
	i.client_exts = NULL;
	i.path="/";

	info.port = CONTEXT_PORT_NO_LISTEN;
	info.protocols = protocols;
	info.gid = -1;
	info.uid = -1;

	context = lws_create_context(&info);
	if (context == NULL) {
		fprintf(stderr, "Creating libwebsocket context failed\n");
		return 1;
	}

	i.context = context;

	if (lws_client_connect_via_info(&i)); // just to prevent warning !!

	forceExit=0;
	// the main magic here !!
	while (!forceExit) {
		lws_service(context, 1000);
	}
	// if there is some errors, we just quit
	lwsl_err("Exiting\n");
	lws_context_destroy(context);

	return 0;
}
