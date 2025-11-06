#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/random.h>


#define DEVICE_NAME "fourinarow"
#define CLASS_NAME "fourinarow_class"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Muhammad Umar");
MODULE_DESCRIPTION("fourinarow");
MODULE_VERSION("0.1");

static int majorNumber;
static char response[1024] = "Hello, this is fourinarow!\n";
static struct class*  fourClass  = NULL;
static struct device* fourDevice = NULL;

static char board[8][8];
static int gameStarted = 0;
static char playerColor = 'Y';
static char computerColor = 'R';
static char currentTurn = 'Y';

// dev_open - Called when the device is opened.
static int dev_open(struct inode *inodep, struct file *filep) {
    //filep->f_flags &= ~O_APPEND;
    //printk(KERN_INFO "fourinarow: Device opened\n");
    return 0;
}

// dev_release - Called when the device is closed.
static int dev_release(struct inode *inodep, struct file *filep) {
    //printk(KERN_INFO "Unmounting Kernel Module\n");
    return 0;
}

// dev_read - Handles reading from the device.
static ssize_t dev_read(struct file *filep, char *buffer, size_t len, loff_t *offset) {
    size_t resp_len = strlen(response);
    if (*offset >= resp_len) return 0;
    if (len > resp_len - *offset) len = resp_len - *offset;

    if (copy_to_user(buffer, response + *offset, len) != 0) return -EFAULT; // Reads from userspace
    *offset += len;
    return len;
}

// four_devnode - Sets device permissions when created.
static char *four_devnode(struct device *dev, umode_t *mode) {
    if (mode)
        *mode = 0666; // read/write permission for everyone
    return NULL;
}

// ResetBoard - Resets the game board to initial empty state.
static void ResetBoard(void) {
    int i, j;
    for (i = 0; i < 8; i++) {
        for (j = 0; j < 8; j++) {
            board[i][j] = 0;
        }
    }
    printk(KERN_INFO "Board has been reset.\n");
}

// checkTie - Checks if the game board is full and it's a tie.
static int checkTie(void) {
    int i, j;
    for (i = 0; i < 8; i++)
        for (j = 0; j < 8; j++)
            if (board[i][j] == 0)
                return 0; // Checks for empty spots
    return 1;
}

// checkWin - Checks if a player has won the game.
static int checkWin(char c) {
    int i, j;
    // Check horizontal wins (left to right)
    for (i = 0; i < 8; i++)
        for (j = 0; j < 5; j++)
            if (board[i][j] == c && board[i][j+1] == c && board[i][j+2] == c && board[i][j+3] == c)
                return 1;
    // Check vertical wins (bottom to top)
    for (i = 0; i < 5; i++)
        for (j = 0; j < 8; j++)
            if (board[i][j] == c && board[i+1][j] == c && board[i+2][j] == c && board[i+3][j] == c)
                return 1;
    // Check diagonal wins (bottom-left to top-right)
    for (i = 0; i < 5; i++)
        for (j = 0; j < 5; j++)
            if (board[i][j] == c && board[i+1][j+1] == c && board[i+2][j+2] == c && board[i+3][j+3] == c)
                return 1;
    
    // Check diagonal wins (bottom-right to top-left)
    for (i = 3; i < 8; i++)
        for (j = 0; j < 5; j++)
            if (board[i][j] == c && board[i-1][j+1] == c && board[i-2][j+2] == c && board[i-3][j+3] == c)
                return 1;

    return 0;
}

 // dev_write - Handles writing commands to the device
static ssize_t dev_write(struct file *filep, const char *buffer, size_t len, loff_t *offset) {
    char cmd[100];
    if (len >= sizeof(cmd)) len = sizeof(cmd) - 1;

    if (copy_from_user(cmd, buffer, len)) return -EFAULT;
    cmd[len] = '\0';
    int i, j;
    if (strncmp(cmd, "RESET", 5) == 0) {
        if (len < 7 || (cmd[6] != 'R' && cmd[6] != 'Y')) {
            snprintf(response, sizeof(response), "Error Invalid Color : Use 'Y' or 'R'\n");
            printk(KERN_INFO "INVALID COLOR\n");
        } else {
            playerColor = cmd[6];
            computerColor = (playerColor == 'Y') ? 'R' : 'Y';
            currentTurn = 'Y';
            gameStarted = 1;
            ResetBoard();
            snprintf(response, sizeof(response), "OK\n");
            printk(KERN_INFO "Color %d\n", playerColor);
        }

    } else if (strncmp(cmd, "BOARD", 5) == 0) {
        int i, j;
        char *p = response;
        p += sprintf(p, " -- A B C D E F G H\n");
        p += sprintf(p, " - - - - - - - - - -\n");
        for (i = 7; i >= 0; i--) {
            p += sprintf(p, "%d |", i + 1);
            for (j = 0; j < 8; j++) {
                p += sprintf(p, " %c", board[i][j] ? board[i][j] : '.');
            }
            p += sprintf(p, "\n");
        }
        *p = '\0';

    } else if (strncmp(cmd, "DROPC", 5) == 0) {
        if (!gameStarted) {
            snprintf(response, sizeof(response), "NOGAME\n");
        } else if (currentTurn != playerColor) {
            snprintf(response, sizeof(response), "OOT\n");
        } else {
            if (len < 7 || cmd[6] < 'A' || cmd[6] > 'H') {
                snprintf(response, sizeof(response), "ERROR\n");
            } else {
                int col = cmd[6] - 'A';
                int row = -1;

                for (i = 0; i < 8; i++) {
                    if (row == -1 && board[i][col] == 0) {
                        board[i][col] = playerColor;
                        row = i;
                    }
                }
                
                printk(KERN_INFO "Attempting to drop: %c\n", cmd[6]);

                if (row == -1) {
                    snprintf(response, sizeof(response), "ERROR\n");
                    printk(KERN_INFO "ERROR\n");
                } else if (checkWin(playerColor)) {
                    gameStarted = 0;
                    snprintf(response, sizeof(response), "WIN\n");
                    printk(KERN_INFO "WIN\n");
                } else if (checkTie()) {
                    gameStarted = 0;
                    snprintf(response, sizeof(response), "TIE\n");
                    printk(KERN_INFO "TIE\n");
                } else {
                    currentTurn = computerColor;
                    snprintf(response, sizeof(response), "OK\n");
                }
            }
        }

    } else if (strncmp(cmd, "CTURN", 5) == 0) {
    if (!gameStarted) {
        snprintf(response, sizeof(response), "NOGAME\n");
    } else if (currentTurn == playerColor) {
        snprintf(response, sizeof(response), "OOT\n");
    } else {
        int col, row = -1, tries, i;
        for (tries = 0; tries < 100 && row == -1; tries++) {
            get_random_bytes(&col, sizeof(col));
            col = col & 7;
            for (i = 0; i < 8 && row == -1; i++) {
                if (board[i][col] == 0) {
                    row = i;
                    board[i][col] = computerColor;
                }
            }
        }

        if (row == -1) {
            snprintf(response, sizeof(response), "ERROR\n");
        } else if (checkWin(computerColor)) {
            gameStarted = 0;
            snprintf(response, sizeof(response), "LOSE\n");
            printk(KERN_INFO "LOSE\n");
        } else if (checkTie()) {
            gameStarted = 0;
            snprintf(response, sizeof(response), "TIE\n");
            printk(KERN_INFO "TIE\n");
        } else {
            currentTurn = playerColor;
            snprintf(response, sizeof(response), "OK\n");
        }
      }
    } else {
        snprintf(response, sizeof(response), "UNKNOWN\n");
    }

    return len;
}



static struct file_operations fops = {
    .open = dev_open,
    .read = dev_read,
    .write = dev_write,
    .release = dev_release,
};

// four_init - Initializes the kernel module, registers the character device,
//             and creates the class and device node
static int __init four_init(void) {
    printk(KERN_INFO "fourinarow: Initializing\n");

    majorNumber = register_chrdev(0, DEVICE_NAME, &fops);
    if (majorNumber < 0) {
        printk(KERN_ALERT "fourinarow: Failed to register major number\n");
        return majorNumber;
    }

    fourClass = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(fourClass)) {
        unregister_chrdev(majorNumber, DEVICE_NAME);
        printk(KERN_ALERT "fourinarow: Failed to register device class\n");
        return PTR_ERR(fourClass);
    }

    // permission handler
    fourClass->devnode = four_devnode;

    fourDevice = device_create(fourClass, NULL, MKDEV(majorNumber, 0), NULL, DEVICE_NAME);
    if (IS_ERR(fourDevice)) {
        class_destroy(fourClass);
        unregister_chrdev(majorNumber, DEVICE_NAME);
        printk(KERN_ALERT "fourinarow: Failed to create the device\n");
        return PTR_ERR(fourDevice);
    }

    printk(KERN_INFO "fourinarow: Device created successfully with major number %d\n", majorNumber);
    ResetBoard();
    return 0;
}

//  four_exit - Cleans up the kernel module, destroys the device and class,
//              and unregisters the character device.
static void __exit four_exit(void) {
    device_destroy(fourClass, MKDEV(majorNumber, 0));
    class_unregister(fourClass);
    class_destroy(fourClass);
    unregister_chrdev(majorNumber, DEVICE_NAME);
    printk(KERN_INFO "Unmounting Kernel Module\n");
}

module_init(four_init);
module_exit(four_exit);
