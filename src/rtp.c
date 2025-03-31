#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "queue.h"
#include "network.h"
#include "rtp.h"

/**
 * PLESE ENTER YOUR INFORMATION BELOW TO RECEIVE MANUAL GRADING CREDITS
 * Name: AVNI MADHWESH
 * GTID: 903727995
 * Fall 2024
 */

typedef struct message
{
    char *buffer;
    int length;
} message_t;

/* ================================================================ */
/*                  H E L P E R    F U N C T I O N S                */
/* ================================================================ */

/**
 * --------------------------------- PROBLEM 1 --------------------------------------
 *
 * Convert the given buffer into an array of PACKETs and returns the array.  The
 * value of (*count) should be updated so that it contains the number of packets in
 * the array. Keep in mind that if length % MAX_PAYLOAD_LENGTH != 0, you might have
 * to manually specify the payload_length.
 *
 * Hint: can we use a heap function to make space for the packets? How many packets?
 *
 * @param buffer pointer to message buffer to be broken up packets
 * @param length length of the message buffer.
 * @param count number of packets in the returning array
 *
 * @returns array of packets
 */
packet_t *packetize(char *buffer, int length, int *count)
{

    packet_t *packets;

    /* ----  FIXME  ---- */
    // Using the 'heap' refers to dynamic memory allocation via malloc() or calloc().

    // Don't worry about freeing the allocated memory you make for the packet array, this is already taken care of for you.

    // 'payload' refers to the data that is being stored within the packet. Recall that a packet consists of a header (metadata) and its actual data (payload).

    // Look at the packet_t struct within network.h to figure out what fields you need to update for each packet. Make sure that the last packet for your message is set to its appropriate packet type.

    // You can find out the number of packets that are needed for the message using the length parameter and MAX_PAYLOAD_SIZE. Hint: The number of packets you want to allocate may depend on whether length % MAX_PAYLOAD_SIZE == 0.

    // If length % MAX_PAYLOAD_SIZE is not 0, then one of the packets will not have the full packet payload size. Hint: You're already doing something special for this packet.

    // For the checksum attribute of the packet, just call the checksum() function. This function has 2 parameters, a pointer to the character buffer that will be used to compute the checksum and the length of the buffer that will be used for the checksum. 

    // The length parameter will just be the same as the size of the packet's payload.

    // Since you only are given a pointer to the start of the character buffer for the entire message, you will need to do some math to calculate how much to shift this starting point to find where the portion of the buffer that corresponds to a packet is. Hint: You may find the packet's index in the array and MAX_PAYLOAD_SIZE useful for this calculation.

    // You will also use the starting point of the character buffer to update the char payload[] attribute for each packet. You may find memcpy(void* dest, void* src, size_t numBytes) useful for copying over the characters from the buffer into the payload array.

    // Remember to update the value at the count pointer to the number of packets in this function!

    int numPacks = length / MAX_PAYLOAD_LENGTH + (length % MAX_PAYLOAD_LENGTH != 0);

    *count = 0;
    packets = (packet_t*) malloc(sizeof(packet_t) * (size_t) numPacks); 
    *count = numPacks;

    for(int i = 0; i < numPacks; i++) { 
        if (i == numPacks - 1) {  //last packet
            packets[i].payload_length = length - i * MAX_PAYLOAD_LENGTH; 
            packets[i].type = LAST_DATA;
        } else {
            packets[i].type = DATA; 
            packets[i].payload_length = MAX_PAYLOAD_LENGTH; 
        }
        memcpy(packets[i].payload, buffer + (i * MAX_PAYLOAD_LENGTH), (size_t)packets[i].payload_length); //incase size changes
        packets[i].checksum = checksum(packets[i].payload, packets[i].payload_length); 

        //logs, delete later
        printf("Packet %d created: Type=%d, Payload='%.*s', Checksum=%d\n",
               i + 1, packets[i].type, packets[i].payload_length, packets[i].payload, packets[i].checksum);
    }
    //*count = numPacks; 

    return packets;
}

/**
 * --------------------------------- PROBLEM 2 --------------------------------------
 *
 * Compute a checksum based on the data in the buffer.
 *
 * Checksum calcuation:
 * Suppose we are given a string in the form
 *      "a b c d e f g h"
 * Then swap each pair of characters such that the characters appear in the form
 *      "b a d c f e h g"
 *
 * If the length of the string is odd, the last character will remain in the same place.
 *
 * i.e. reorder the string such that each pair of characters are swapped.
 * Following this, the checksum will be equal to the sum of the ASCII values of each
 * character multiplied by it's index in the string.
 *
 * @param buffer pointer to the char buffer that the checksum is calculated from
 * @param length length of the buffer
 *
 * @returns calcuated checksum
 */
int checksum(char *buffer, int length)
{

    /* ----  FIXME  ---- */
    int sum = 0;
    if (buffer == NULL || length <= 0) {
        return 0;
    }

    char *temp_buffer = malloc((size_t)length);

    if (temp_buffer == NULL) {
        // Handle memory allocation failure
        fprintf(stderr, "Memory allocation failed in checksum()\n");
        return 0;
    }

    memcpy(temp_buffer, buffer, (size_t)length);

    for (int i = 0; i < length; i++) {
        if (i % 2 == 0 && i + 1 < length) {
            // Swap character pairs
            char temp = temp_buffer[i];
            temp_buffer[i] = temp_buffer[i + 1];
            temp_buffer[i + 1] = temp;
        }
        // Calculate checksum
        sum += (int)temp_buffer[i] * i;
    }

    free(temp_buffer);
    return sum;

//     return 0;
}

/* ================================================================ */
/*                      R T P       T H R E A D S                   */
/* ================================================================ */

static void *rtp_recv_thread(void *void_ptr)
{

    rtp_connection_t *connection = (rtp_connection_t *)void_ptr;

    do
    {
        message_t *message;
        int buffer_length = 0;
        char *buffer = NULL;
        packet_t packet;

        /* Put messages in buffer until the last packet is received  */
        do
        {
            if (net_recv_packet(connection->net_connection_handle, &packet) <= 0 || packet.type == TERM)
            {
                /* remote side has disconnected */
                connection->alive = 0;
                pthread_cond_signal(&connection->recv_cond);
                pthread_cond_signal(&connection->send_cond);
                break;
            }

            /*  ----  FIXME: Part III-A ----
             *
             * 1. Check to make sure payload of packet is correct
             * 2. Send an ACK or a NACK, whichever is appropriate
             * 3. If this is the last packet in a sequence of packets
             *    and the payload was corrupted, make sure the loop
             *    does not terminate
             * 4. If the payload matches, add the payload to the buffer
             */

// The given code is a bit difficult to follow here, but know the variable 'packet' already has the packet you're looking for. Also note that packet is of type packet_t, not packet_t*. 

// The connection you're using is referenced by the pointer 'connection'. All the mutexes/conditions that you will be using for all of problem 3 will be through your connection pointer.

// If the packet is a DATA or LAST_DATA packet, you need to calculate the checksum of the received payload and compare it to the packet's payload attribute. If the checksums are identical, then you want to copy over the payload of the packet into the buffer, and then send an ACK packet. Otherwise, you need to send a NACK packet.

// Sending the packets can be done using the net_send_packet() function. The first parameter of this function takes the net_connection_handle of the rtp_connection. The second parameter of this function takes a pointer to the packet you want to send.

// You will need to allocate/reallocate memory every time you add to the buffer (in this implementation, the receiver has no way of knowing ahead of time how big the message will be).

// Like before, you can use memcpy() to copy the payload of a packet into the buffer. Note that you will need to shift the starting point of the buffer as it grows, so that you don't copy over areas that have already been written to. You may find the variable buffer_length useful for calculating the shift, provided you keep it updated. (bonus hint: update the buffer_length after you copy the payload over, rather than before)

// You want to ensure that if the last packet is corrupted, the loop does not terminate. Since the given loop terminates when the type of the packet is LAST_DATA, you can simply change the type of the packet (when the checksum doesn't match) before you reach the loop check to avoid this problem. When the loop restarts, you will get a new packet anyways, so this doesn't cause any problems.

            if (packet.type == DATA || packet.type == LAST_DATA) { 
                packet_t *msg = malloc(sizeof(*msg));

                if (checksum(packet.payload, packet.payload_length) == packet.checksum) { 
                    msg->type = ACK; 
                    int newsize = buffer_length * packet.payload_length;
                    buffer = realloc(buffer, (size_t) (newsize));
                    memcpy(buffer + buffer_length, packet.payload, (size_t) packet.payload_length);
                    buffer_length = buffer_length + packet.payload_length; 
                    } else {
                        msg->type = NACK; 
                        if (packet.type == LAST_DATA) { 
                            packet.type = DATA; 
                        }
                    }

                net_send_packet(connection->net_connection_handle, msg); 
                free(msg); 
            }

            /*
             *  What if the packet received is not a data packet?
             *  If it is a NACK or an ACK, the sending thread should
             *  be notified so that it can finish sending the message.
             *
             *  1. Add the necessary fields to the CONNECTION data structure
             *     in rtp.h so that the sending thread has a way to determine
             *     whether a NACK or an ACK was received
             *  2. Signal the sending thread that an ACK or a NACK has been
             *     received.
             */

// If the packet is instead an ACK or NACK packet, you need to send the appropriate signal to tell the sender that the ACK/NACK packet has been acknowledged. (You don't need to worry about the case where the packet is of type TERM, that is already handled for you).

// You need to add a variable to the rtp_connection struct in rtp.h to differentiate whether a NACK or ACK was received. Hint: What type of variable was used in the packet_struct to store the type of the packet? This may help you determine what type of variable to add to the struct.

// You can set a default value for the type to say that your sender is waiting for your receiver to read the ACK or NACK packet. Instead, you should also add a variable representing one bit to specify if the ack packet has been read yet or not. This will become relevant in future steps. 

// When modifying this value within the rtp_connection, make sure that you use the appropriate lock and signal for thread-safe functionality.

            else if (packet.type == ACK) {
                connection->ack_received = ACK; 
                pthread_cond_signal(&(connection->ack_cond)); 
            } else if (packet.type == NACK) { 
                connection->ack_received = NACK; 
                pthread_cond_signal(&(connection->ack_cond));
            } 
        } while (packet.type != LAST_DATA);

        if (connection->alive == 1)
        {
            /*  ----  FIXME: Part III-B ----
             *
             * Now that an entire message has been received, we need to
             * add it to the queue to provide to the rtp client.
             *
             * 1. Add message to the received queue.
             * 2. Signal the client thread that a message has been received.
             */

            message = malloc(sizeof(*message)); 
            message->length = buffer_length;
            message->buffer = malloc((size_t) buffer_length); 
            memcpy(message->buffer, buffer, (size_t) buffer_length); 

            pthread_mutex_lock(&(connection->recv_mutex));
            queue_add(&(connection->recv_queue), message); 
            pthread_cond_signal(&(connection->recv_cond));
            pthread_mutex_unlock(&(connection->recv_mutex)); 
        }
        else
            free(buffer);

    } while (connection->alive == 1);

    return NULL;
}

static void *rtp_send_thread(void *void_ptr)
{

    rtp_connection_t *connection = (rtp_connection_t *)void_ptr;
    message_t *message;
    int array_length = 0;
    int i;
    packet_t *packet_array;

    do
    {
        /* Extract the next message from the send queue */
        pthread_mutex_lock(&connection->send_mutex);
        while (queue_size(&connection->send_queue) == 0 && connection->alive == 1)
        {
            pthread_cond_wait(&connection->send_cond, &connection->send_mutex);
        }

        if (connection->alive == 0)
        {
            pthread_mutex_unlock(&connection->send_mutex);
            break;
        }

        message = queue_extract(&connection->send_queue);

        pthread_mutex_unlock(&connection->send_mutex);

        /* Packetize the message and send it */
        packet_array = packetize(message->buffer, message->length, &array_length);

        for (i = 0; i < array_length; i++)
        {

            //LOGS--> delete these two lines later!=====
            printf("Sending packet %d: Type=%d, Payload='%.*s', Checksum=%d\n",
            i + 1, packet_array[i].type, packet_array[i].payload_length, packet_array[i].payload, packet_array[i].checksum);
           //===========================================

            /* Start sending the packetized messages */
            if (net_send_packet(connection->net_connection_handle, &packet_array[i]) <= 0)
            {
                /* remote side has disconnected */
                connection->alive = 0;
                break;
            }

            /*  ----FIX ME: Part III-C ----
             *
             *  1. Wait for the recv thread to notify you of when a NACK or
             *     an ACK has been received
             *  2. Check the data structure for this connection to determine
             *     if an ACK or NACK was received.  (You'll have to add the
             *     necessary fields yourself)
             *  3. If it was an ACK, continue sending the packets.
             *  4. If it was a NACK, resend the last packet
             */


        //OLD MINE
            pthread_mutex_lock(&connection->ack_mutex);
            while (connection->ack_received == 0) {
                pthread_cond_wait(&connection->ack_cond, &connection->ack_mutex);
            }

            if (connection->ack_received == NACK) {
                printf("NACK received for packet %d. Resending...\n", i + 1);
                i--; 
                
            }

            connection->ack_received = 0; 
            pthread_mutex_unlock(&connection->ack_mutex);
        }

        free(packet_array);
        free(message->buffer);
        free(message);
    } while (connection->alive == 1);
    return NULL;
}

/* ================================================================ */
/*                           R T P    A P I                         */
/* ================================================================ */

static rtp_connection_t *rtp_init_connection(int net_connection_handle)
{
    rtp_connection_t *rtp_connection = malloc(sizeof(rtp_connection_t));

    if (rtp_connection == NULL)
    {
        fprintf(stderr, "Out of memory!\n");
        exit(EXIT_FAILURE);
    }

    rtp_connection->net_connection_handle = net_connection_handle;

    queue_init(&rtp_connection->recv_queue);
    queue_init(&rtp_connection->send_queue);

    pthread_mutex_init(&rtp_connection->ack_mutex, NULL);
    pthread_mutex_init(&rtp_connection->recv_mutex, NULL);
    pthread_mutex_init(&rtp_connection->send_mutex, NULL);
    pthread_cond_init(&rtp_connection->ack_cond, NULL);
    pthread_cond_init(&rtp_connection->recv_cond, NULL);
    pthread_cond_init(&rtp_connection->send_cond, NULL);

    rtp_connection->alive = 1;

    pthread_create(&rtp_connection->recv_thread, NULL, rtp_recv_thread,
                   (void *)rtp_connection);
    pthread_create(&rtp_connection->send_thread, NULL, rtp_send_thread,
                   (void *)rtp_connection);

    return rtp_connection;
}

rtp_connection_t *rtp_connect(char *host, int port)
{

    int net_connection_handle;

    if ((net_connection_handle = net_connect(host, port)) < 1)
        return NULL;

    return (rtp_init_connection(net_connection_handle));
}

int rtp_disconnect(rtp_connection_t *connection)
{

    message_t *message;
    packet_t term;

    term.type = TERM;
    term.payload_length = term.checksum = 0;
    net_send_packet(connection->net_connection_handle, &term);
    connection->alive = 0;

    net_disconnect(connection->net_connection_handle);
    pthread_cond_signal(&connection->send_cond);
    pthread_cond_signal(&connection->recv_cond);
    pthread_join(connection->send_thread, NULL);
    pthread_join(connection->recv_thread, NULL);
    net_release(connection->net_connection_handle);

    /* emtpy recv queue and free allocated memory */
    while ((message = queue_extract(&connection->recv_queue)) != NULL)
    {
        free(message->buffer);
        free(message);
    }
    queue_release(&connection->recv_queue);

    /* emtpy send queue and free allocated memory */
    while ((message = queue_extract(&connection->send_queue)) != NULL)
    {
        free(message);
    }
    queue_release(&connection->send_queue);

    free(connection);

    return 1;
}

int rtp_recv_message(rtp_connection_t *connection, char **buffer, int *length)
{

    message_t *message;

    if (connection->alive == 0)
        return -1;
    /* lock */
    pthread_mutex_lock(&connection->recv_mutex);
    while (queue_size(&connection->recv_queue) == 0 && connection->alive == 1)
    {
        pthread_cond_wait(&connection->recv_cond, &connection->recv_mutex);
    }

    if (connection->alive == 0)
    {
        pthread_mutex_unlock(&connection->recv_mutex);
        return -1;
    }

    /* extract */
    message = queue_extract(&connection->recv_queue);
    *buffer = message->buffer;
    *length = message->length;
    free(message);

    /* unlock */
    pthread_mutex_unlock(&connection->recv_mutex);

    return *length;
}

int rtp_send_message(rtp_connection_t *connection, char *buffer, int length)
{

    message_t *message;

    if (connection->alive == 0)
        return -1;

    message = malloc(sizeof(message_t));
    if (message == NULL)
    {
        return -1;
    }
    message->buffer = malloc((size_t)length);
    message->length = length;

    if (message->buffer == NULL)
    {
        free(message);
        return -1;
    }

    memcpy(message->buffer, buffer, (size_t)length);

    /* lock */
    pthread_mutex_lock(&connection->send_mutex);

    /* add */
    queue_add(&(connection->send_queue), message);

    /* unlock */
    pthread_mutex_unlock(&connection->send_mutex);
    pthread_cond_signal(&connection->send_cond);
    return 1;
}