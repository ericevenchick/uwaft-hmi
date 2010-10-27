#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
 .name = KBUILD_MODNAME,
 .init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
 .exit = cleanup_module,
#endif
 .arch = MODULE_ARCH_INIT,
};

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0xcb63ae9f, "module_layout" },
	{ 0x6bc3fbc0, "__unregister_chrdev" },
	{ 0x77a108df, "_write_unlock_irqrestore" },
	{ 0x4a427495, "per_cpu__current_task" },
	{ 0x12da5bb2, "__kmalloc" },
	{ 0x7871d3ff, "usb_buffer_alloc" },
	{ 0xb279da12, "pv_lock_ops" },
	{ 0x2f3efdcf, "dev_set_drvdata" },
	{ 0xc8b57c27, "autoremove_wake_function" },
	{ 0x71356fba, "remove_wait_queue" },
	{ 0xaab06af8, "_write_lock_irqsave" },
	{ 0x973873ab, "_spin_lock" },
	{ 0xc633495b, "schedule_work" },
	{ 0x9fa54650, "usb_buffer_free" },
	{ 0xeb72a81c, "usb_kill_urb" },
	{ 0xc1b2b816, "remove_proc_entry" },
	{ 0x6729d3df, "__get_user_4" },
	{ 0x35dad892, "queue_work" },
	{ 0xd7bb239d, "__register_chrdev" },
	{ 0x6a9f26c9, "init_timer_key" },
	{ 0x712aa29b, "_spin_lock_irqsave" },
	{ 0x3c2c5af5, "sprintf" },
	{ 0x7d11c268, "jiffies" },
	{ 0xffc7c184, "__init_waitqueue_head" },
	{ 0x9629486a, "per_cpu__cpu_number" },
	{ 0xffd5a395, "default_wake_function" },
	{ 0x1cefe352, "wait_for_completion" },
	{ 0xe83fea1, "del_timer_sync" },
	{ 0x4ae78c98, "usb_deregister" },
	{ 0xb72397d5, "printk" },
	{ 0xecde1418, "_spin_lock_irq" },
	{ 0xacdeb154, "__tracepoint_module_get" },
	{ 0xa1c76e0a, "_cond_resched" },
	{ 0x2f287f0d, "copy_to_user" },
	{ 0xb4390f9a, "mcount" },
	{ 0x73d19c27, "destroy_workqueue" },
	{ 0x4b07e779, "_spin_unlock_irqrestore" },
	{ 0x46085e4f, "add_timer" },
	{ 0x41fd3de, "__create_workqueue_key" },
	{ 0x78a484c9, "_read_unlock_irqrestore" },
	{ 0x7a848702, "_read_lock_irqsave" },
	{ 0xfb010786, "usb_submit_urb" },
	{ 0x8ff4079b, "pv_irq_ops" },
	{ 0xb2fd5ceb, "__put_user_4" },
	{ 0xf0fdf6cb, "__stack_chk_fail" },
	{ 0x3bd1b1f6, "msecs_to_jiffies" },
	{ 0x95b36797, "usb_bulk_msg" },
	{ 0xd62c833f, "schedule_timeout" },
	{ 0x4292364c, "schedule" },
	{ 0x5dc2e9b8, "create_proc_entry" },
	{ 0xeb94dbea, "__module_put_and_exit" },
	{ 0x1a8f0df2, "wake_up_process" },
	{ 0x7ecb001b, "__per_cpu_offset" },
	{ 0x642e54ac, "__wake_up" },
	{ 0x1d2e87c6, "do_gettimeofday" },
	{ 0x650fb346, "add_wait_queue" },
	{ 0x3aa1dbcf, "_spin_unlock_bh" },
	{ 0x37a0cba, "kfree" },
	{ 0xf8de91ef, "kthread_create" },
	{ 0x801678, "flush_scheduled_work" },
	{ 0x33d92f9a, "prepare_to_wait" },
	{ 0x6b5e1c8d, "usb_register_driver" },
	{ 0x9ccb2622, "finish_wait" },
	{ 0xe456bd3a, "complete" },
	{ 0x93cbd1ec, "_spin_lock_bh" },
	{ 0xd6c963c, "copy_from_user" },
	{ 0x5c960482, "dev_get_drvdata" },
	{ 0xf4a75166, "usb_free_urb" },
	{ 0x412fa5e4, "usb_alloc_urb" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";

MODULE_ALIAS("usb:v0BFDp0004d*dc*dsc*dp*ic*isc*ip*");
MODULE_ALIAS("usb:v0BFDp0002d*dc*dsc*dp*ic*isc*ip*");
MODULE_ALIAS("usb:v0BFDp0005d*dc*dsc*dp*ic*isc*ip*");
MODULE_ALIAS("usb:v0BFDp0003d*dc*dsc*dp*ic*isc*ip*");

MODULE_INFO(srcversion, "364627ACB879484B2BDA734");
