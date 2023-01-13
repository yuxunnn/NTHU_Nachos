#include <assert.h>
#include <stdlib.h>
#include "ts_queue.hpp"
#include "item.hpp"
#include "reader.hpp"
#include "writer.hpp"
#include "producer.hpp"
#include "consumer_controller.hpp"

#define READER_QUEUE_SIZE 200
#define WORKER_QUEUE_SIZE 200
#define WRITER_QUEUE_SIZE 4000
#define CONSUMER_CONTROLLER_LOW_THRESHOLD_PERCENTAGE 20
#define CONSUMER_CONTROLLER_HIGH_THRESHOLD_PERCENTAGE 80
#define CONSUMER_CONTROLLER_CHECK_PERIOD 1000000

int main(int argc, char** argv) {
	assert(argc == 4);

	int n = atoi(argv[1]);
	std::string input_file_name(argv[2]);
	std::string output_file_name(argv[3]);

	// TODO: implements main function
	TSQueue<Item*>* input_queue = new TSQueue<Item*>(READER_QUEUE_SIZE);
	TSQueue<Item*>* worker_queue = new TSQueue<Item*>(WORKER_QUEUE_SIZE);
	TSQueue<Item*>* output_queue = new TSQueue<Item*>(WRITER_QUEUE_SIZE);

	/* Create */
	Transformer* transformer = new Transformer;
	Reader* reader = new Reader(n, input_file_name, input_queue);
	Producer* producer1 = new Producer(input_queue, worker_queue, transformer);
	Producer* producer2 = new Producer(input_queue, worker_queue, transformer);
	Producer* producer3 = new Producer(input_queue, worker_queue, transformer);
	Producer* producer4 = new Producer(input_queue, worker_queue, transformer);
	ConsumerController* consumer_controller = new ConsumerController(worker_queue, output_queue, transformer, 
												CONSUMER_CONTROLLER_CHECK_PERIOD * WORKER_QUEUE_SIZE / 100, 
												CONSUMER_CONTROLLER_LOW_THRESHOLD_PERCENTAGE * WORKER_QUEUE_SIZE / 100, 
												CONSUMER_CONTROLLER_HIGH_THRESHOLD_PERCENTAGE);
	Writer* writer = new Writer(n, output_file_name, output_queue);

	/* start */
	reader->start();
	producer1->start();
	producer2->start();
	producer3->start();
	producer4->start();
	consumer_controller->start();
	writer->start();

	/* wait for finish */
	reader->join();
	writer->join();

	/* delete */
	delete input_queue;
	delete worker_queue;
	delete output_queue;
	delete transformer;
	delete reader;
	delete producer1;
	delete producer2;
	delete producer3;
	delete producer4;
	delete consumer_controller;
	delete writer;
	
	return 0;
}
