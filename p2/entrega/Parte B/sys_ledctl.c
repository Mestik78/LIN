#include <linux/syscalls.h>
#include <linux/vt_kern.h>
#include <linux/tty.h>      /* For fg_console */
#include <linux/kd.h>       /* For KDSETLED */

SYSCALL_DEFINE1(ledctl, unsigned int, leds)
{
    unsigned int hw_mask = 0;
    
    if (leds > 7)
        return -EINVAL;

    if (leds & 0x1) hw_mask |= 0x1;
    if (leds & 0x2) hw_mask |= 0x4;
    if (leds & 0x4) hw_mask |= 0x2;

    return (vc_cons[fg_console].d->port.tty->driver->ops->ioctl) (vc_cons[fg_console].d->port.tty, KDSETLED, hw_mask);
}
