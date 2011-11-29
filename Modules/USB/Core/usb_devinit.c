/*
 * Acess 2 USB Stack
 * - By John Hodge (thePowersGang)
 *
 * usb_devinit.c
 * - USB Device Initialisation
 */
#define DEBUG	1

#include <acess.h>
#include <vfs.h>
#include <drv_pci.h>
#include "usb.h"
#include "usb_proto.h"
#include "usb_lowlevel.h"

// === PROTOTYPES ===
void	USB_DeviceConnected(tUSBHub *Hub, int Port);
void	USB_DeviceDisconnected(tUSBHub *Hub, int Port);
void	*USB_GetDeviceDataPtr(tUSBInterface *Dev);
void	USB_SetDeviceDataPtr(tUSBInterface *Dev, void *Ptr);
 int	USB_int_AllocateAddress(tUSBHost *Host);

// === CODE ===
void USB_DeviceConnected(tUSBHub *Hub, int Port)
{
	tUSBDevice	tmpdev;
	tUSBDevice	*dev = &tmpdev;
	if( Port >= Hub->nPorts )	return ;
	if( Hub->Devices[Port] )	return ;

	ENTER("pHub iPort", Hub, Port);

	// Device should be in 'Default' state
	
	// Create structure
	dev->ParentHub = Hub;
	dev->Host = Hub->Interface->Dev->Host;
	dev->Address = 0;

	// 1. Assign an address
	dev->Address = USB_int_AllocateAddress(dev->Host);
	if(dev->Address == 0) {
		Log_Error("USB", "No addresses avaliable on host %p", dev->Host);
		free(dev);
		LEAVE('-');
		return ;
	}
	USB_int_SendSetupSetAddress(dev->Host, dev->Address);
	LOG("Assigned address %i", dev->Address);
	
	// 2. Get device information
	{
		struct sDescriptor_Device	desc;
		LOG("Getting device descriptor");
		// Endpoint 0, Desc Type 1, Index 0
		USB_int_ReadDescriptor(dev, 0, 1, 0, sizeof(desc), &desc);
		
		LOG("Device Descriptor = {");
		LOG(" .Length = %i", desc.Length);
		LOG(" .Type = %i", desc.Type);
		LOG(" .USBVersion = 0x%04x", desc.USBVersion);
		LOG(" .DeviceClass = 0x%02x", desc.DeviceClass);
		LOG(" .DeviceSubClass = 0x%02x", desc.DeviceSubClass);
		LOG(" .DeviceProtocol = 0x%02x", desc.DeviceProtocol);
		LOG(" .MaxPacketSize = 0x%02x", desc.MaxPacketSize);
		LOG(" .VendorID = 0x%04x", desc.VendorID);
		LOG(" .ProductID = 0x%04x", desc.ProductID);
		LOG(" .DeviceID = 0x%04x", desc.DeviceID);
		LOG(" .ManufacturerStr = Str %i", desc.ManufacturerStr);
		LOG(" .ProductStr = Str %i", desc.ProductStr);
		LOG(" .SerialNumberStr = Str %i", desc.SerialNumberStr);
		LOG(" .NumConfigurations = %i", desc.SerialNumberStr);
		LOG("}");
		
		if( desc.ManufacturerStr )
		{
			char	*tmp = USB_int_GetDeviceString(dev, 0, desc.ManufacturerStr);
			LOG("ManufacturerStr = '%s'", tmp);
			free(tmp);
		}
		if( desc.ProductStr )
		{
			char	*tmp = USB_int_GetDeviceString(dev, 0, desc.ProductStr);
			LOG("ProductStr = '%s'", tmp);
			free(tmp);
		}
		if( desc.SerialNumberStr )
		{
			char	*tmp = USB_int_GetDeviceString(dev, 0, desc.SerialNumberStr);
			LOG("SerialNumbertStr = '%s'", tmp);
			free(tmp);
		}
	}

	// TODO: Support alternate configurations
	
	// 3. Get configurations
	for( int i = 0; i < 1; i ++ )
	{
		struct sDescriptor_Configuration	desc;
		void	*full_buf;
		char	*cur_ptr;
	
		USB_int_ReadDescriptor(dev, 0, 2, i, sizeof(desc), &desc);
		LOG("Configuration Descriptor %i = {", i);
		LOG(" .Length = %i", desc.Length);
		LOG(" .Type = %i", desc.Type);
		LOG(" .TotalLength = 0x%x", LittleEndian16(desc.TotalLength));
		LOG(" .NumInterfaces = %i", desc.NumInterfaces);
		LOG(" .ConfigurationValue = %i", desc.ConfigurationValue);
		LOG(" .ConfigurationStr = %i", desc.ConfigurationStr);
		LOG(" .AttributesBmp = 0b%b", desc.AttributesBmp);
		LOG(" .MaxPower = %i (*2mA)", desc.MaxPower);
		LOG("}");
		if( desc.ConfigurationStr ) {
			char	*tmp = USB_int_GetDeviceString(dev, 0, desc.ConfigurationStr);
			LOG("ConfigurationStr = '%s'", tmp);
			free(tmp);
		}

		// TODO: Split here and allow some method of selection

		// Allocate device now that we have the configuration
		dev = malloc(sizeof(tUSBDevice) + desc.NumInterfaces * sizeof(void*));
		memcpy(dev, &tmpdev, sizeof(tUSBDevice));
		dev->nInterfaces = desc.NumInterfaces;
	
		// Allocate a temp buffer for config info
		cur_ptr = full_buf = malloc( LittleEndian16(desc.TotalLength) );
		USB_int_ReadDescriptor(dev, 0, 2, i, desc.TotalLength, full_buf);

		cur_ptr += desc.Length;

		// TODO: Interfaces
		for( int j = 0; j < desc.NumInterfaces; j ++ )
		{
			struct sDescriptor_Interface *iface;
			tUSBInterface	*dev_if;
			iface = (void*)cur_ptr;
			// TODO: Sanity check with remaining space
			cur_ptr += sizeof(*iface);

			LOG("Interface %i/%i = {", i, j);
			LOG(" .InterfaceNum = %i", iface->InterfaceNum);
			LOG(" .NumEndpoints = %i", iface->NumEndpoints);
			LOG(" .InterfaceClass = 0x%x", iface->InterfaceClass);
			LOG(" .InterfaceSubClass = 0x%x", iface->InterfaceSubClass);
			LOG(" .InterfaceProcol = 0x%x", iface->InterfaceProtocol);

			if( iface->InterfaceStr ) {
				char	*tmp = USB_int_GetDeviceString(dev, 0, iface->InterfaceStr);
				LOG(" .InterfaceStr = %i '%s'", iface->InterfaceStr, tmp);
				free(tmp);
			}
			LOG("}");

			dev_if = malloc(sizeof(tUSBInterface) + iface->NumEndpoints*sizeof(dev_if->Endpoints[0]));
			dev_if->Dev = dev;
			dev_if->Driver = NULL;
			dev_if->Data = NULL;
			dev_if->nEndpoints = iface->NumEndpoints;
			dev->Interfaces[j] = dev_if;

			// Copy interface data
			for( int k = 0; k < iface->NumEndpoints; k ++ )
			{
				struct sDescriptor_Endpoint *endpt;
				endpt = (void*)cur_ptr;
				// TODO: Sanity check with remaining space
				cur_ptr += sizeof(*endpt);
				
				LOG("Endpoint %i/%i/%i = {", i, j, k);
				LOG(" .Address = 0x%2x", endpt->Address);
				LOG(" .Attributes = 0b%8b", endpt->Attributes);
				LOG(" .MaxPacketSize = %i", LittleEndian16(endpt->MaxPacketSize));
				LOG(" .PollingInterval = %i", endpt->PollingInterval);
				LOG("}");
				
				dev_if->Endpoints[k].Next = NULL;
				dev_if->Endpoints[k].Interface = dev_if;
				dev_if->Endpoints[k].EndpointIdx = k;
				dev_if->Endpoints[k].EndpointNum = endpt->Address & 0x7F;
				dev_if->Endpoints[k].PollingPeriod = endpt->PollingInterval;
				dev_if->Endpoints[k].MaxPacketSize = LittleEndian16(endpt->MaxPacketSize);
				dev_if->Endpoints[k].Type = endpt->Attributes | (endpt->Address & 0x80);
				dev_if->Endpoints[k].PollingAtoms = 0;
				dev_if->Endpoints[k].InputData = NULL;
			}
			
			// Initialise driver
			dev_if->Driver = USB_int_FindDriverByClass(
				 ((int)iface->InterfaceClass << 16)
				|((int)iface->InterfaceSubClass << 8)
				|((int)iface->InterfaceProtocol << 0)
				);
			if(!dev_if->Driver) {
				Log_Notice("USB", "No driver for Class %02x:%02x:%02x",
					iface->InterfaceClass, iface->InterfaceSubClass, iface->InterfaceProtocol
					);
			}
			else {
				dev_if->Driver->Connected( dev_if );
			}
		}
		
		free(full_buf);
	}

	// Done.
	LEAVE('-');
}

void USB_DeviceDisconnected(tUSBHub *Hub, int Port)
{
	
}

void *USB_GetDeviceDataPtr(tUSBInterface *Dev) { return Dev->Data; }
void USB_SetDeviceDataPtr(tUSBInterface *Dev, void *Ptr) { Dev->Data = Ptr; }

int USB_int_AllocateAddress(tUSBHost *Host)
{
	 int	i;
	for( i = 1; i < 128; i ++ )
	{
		if(Host->AddressBitmap[i/8] & (1 << (i%8)))
			continue ;
		Host->AddressBitmap[i/8] |= 1 << (i%8);
		return i;
	}
	return 0;
}

void USB_int_DeallocateAddress(tUSBHost *Host, int Address)
{
	Host->AddressBitmap[Address/8] &= ~(1 << (Address%8));
}

