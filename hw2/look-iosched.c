/*
 *elevator c-look
 */
#include <linux/blkdev.h>
#include <linux/elevator.h>
#include <linux/bio.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/init.h>

sector_t currSector = -1;

struct look_data{
	struct list_head queue;
};

static void look_merged_requests(struct request_queue *q, struct request *rq, struct request *next)
{
	list_del_init(&next->queuelist);
	printk(KERN_DEBUG "Request merged.\n");
}

static int look_dispatch(struct request_queue *q, int force)
{
	struct look_data *nd = q->elevator->elevator_data;

	if(!list_empty(&nd->queue)){
		struct request *rq;
		rq = list_entry(nd->queue.next, struct request, queuelist);
		list_del_init(&rq->queuelist);
		elv_dispatch_sort(q, rq);
		currSector = blk_rq_pos(rq);
		return 1;
	}
	printk(KERN_DEBUG "Queue dispatched.\n");
	return 0;
}

static int look_init_queue(struct request_queue *q, struct elevator_type *e)
{
	struct look_data *nd;
	struct elevator_queue *eq;

	eq = elevator_alloc(q, e);
	if(!eq){
		return -ENOMEM;
	}
	nd = kmalloc_node(sizeof(*nd), GFP_KERNEL, q->node);
	if(!nd){
		kobject_put(&eq->kobj);
		return -ENOMEM;
	}
	eq->elevator_data = nd;

	INIT_LIST_HEAD(&nd->queue);

	spin_lock_irq(q->queue_lock);
	q->elevator = eq;
	spin_unlock_irq(q->queue_lock);
	printk(KERN_DEBUG "SSTF INIT.\n");
	return 0;
}

static void look_add_request(struct request_queue *q, struct request *rq)
{
	
    struct look_data *nd = q->elevator->elevator_data;
	struct list_head *curr = NULL;
	struct list_head *reqOnQueue = NULL;

	if(list_empty(&nd->queue)){
		printk(KERN_DEBUG "EM: The queue is empty.\n");
	}

	list_for_each(curr, &nd->queue){
		struct request *c = list_entry(curr, struct request, queuelist);
		printk(KERN_DEBUG "Now going through the queue.\n");
		
        if(blk_rq_pos(rq)<currSector){
			if(blk_rq_pos(rq)>blk_rq_pos(c)){
				break;
			}
			if(blk_rq_pos(c)>currSector){
				break;
			}
		}else{
			if(blk_rq_pos(c)>currSector && blk_rq_pos(rq)>blk_rq_pos(c)){
				break;
			}
		}
	}

	if(curr != NULL){
		printk(KERN_DEBUG "Current is not NULL!!");
	}

	list_add_tail(&rq->queuelist, curr);

	printk(KERN_DEBUG "This is current sector: %llu.\n", (unsigned long long)currSector);//original 
	printk(KERN_DEBUG "This is the sector of the new request: %llu.\n", (unsigned long long)blk_rq_pos(rq));
	list_for_each(reqOnQueue, &nd->queue){
		struct request *secNum = list_entry(reqOnQueue, struct request, queuelist);
		printk(KERN_DEBUG "%llu.\n", (unsigned long long)blk_rq_pos(secNum));
	}
	printk(KERN_DEBUG "Request Added.\n");
}

static struct request* look_latter_request(struct request_queue *q, struct request *rq)
{
	struct look_data *nd = q->elevator->elevator_data;

	if(rq->queuelist.next == &nd->queue){
		return NULL;
	}
	return list_entry(rq->queuelist.next, struct request, queuelist);
}

static struct request* look_former_request(struct request_queue *q, struct request *rq)
{
	struct look_data *nd = q->elevator->elevator_data;

	if(rq->queuelist.prev == &nd->queue){
		return NULL;
	}
	return list_entry(rq->queuelist.prev, struct request, queuelist);
}

static void look_exit_queue(struct elevator_queue *e)
{
	struct look_data *nd = e->elevator_data;

	BUG_ON(!list_empty(&nd->queue));
	kfree(nd);
}

static struct elevator_type elevator_look = {
	.ops = {
		.elevator_merge_req_fn		= look_merged_requests,
		.elevator_dispatch_fn		= look_dispatch,
		.elevator_add_req_fn		= look_add_request,
		.elevator_former_req_fn		= look_former_request,
		.elevator_latter_req_fn		= look_latter_request,
		.elevator_init_fn		= look_init_queue,
		.elevator_exit_fn		= look_exit_queue,
	},
	.elevator_name = "look",
	.elevator_owner = THIS_MODULE,
};

static int __init look_init(void)
{
	return elv_register(&elevator_look);
}

static void __exit look_exit(void)
{
	elv_unregister(&elevator_look);
}

module_init(look_init);
module_exit(look_exit);

MODULE_AUTHOR("Tao Chen");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("C-Look IO Scheduler");
