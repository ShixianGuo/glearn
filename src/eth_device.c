
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>

#include <linux/inetdevice.h>




static int	nic_open(struct net_device *dev){

}

static int nic_stop(struct net_device *dev){

}


static int nic_change_mtu(struct net_device *dev, int new_mtu) {
	struct nic_priv *priv = netdev_priv(dev);
	netif_info(priv, drv, dev, "%s(#%d), priv:%p, mtu%d\n",
                __func__, __LINE__, priv, new_mtu);

	return eth_change_mtu(dev, new_mtu);
}

static struct net_device*nic_dev;
static struct net_device_ops * nic_dev_ops={
    .ndo_open = nic_open,
	.ndo_stop = nic_stop

	
};