#include <ntddk.h>
#include <dontuse.h>

//NOTE: NO SYMBOLIC LINK YET. THIS DRIVER IS STILL IN DEVELOPMENT. 
void latopUnload(PDRIVER_OBJECT DriverObject);
NTSTATUS PassThru(PDEVICE_OBJECT DeviceObject, PIRP irp);
NTSTATUS ReadKeyboardInput(PDEVICE_OBJECT DeviceObject, PIRP irp);
NTSTATUS DeviceAttach(PDRIVER_OBJECT DeviceObject, PDEVICE_OBJECT ActualKeyboard);
NTSTATUS CompletionRoutine(PDEVICE_OBJECT DeviceObject, PIRP irp, PVOID Context);

typedef struct _DEVICE_EXTENSION { //this is a private storage box for my driver. it'll store the address of LowerDeviceObject. 

	PDEVICE_OBJECT LowerDeviceObject; //next device in the stack

}DEVICE_EXTENSION, * PDEVICE_EXTENSION;

typedef struct _MY_COMPLETION_CONTEXT {
	KPRIORITY oldprio;
	KAFFINITY oldaffinity;
} MY_COMPLETION_CONTEXT, *PMY_COMPLETION_CONTEXT;


extern "C" NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING registrypath ) { //FINISHED
	NTSTATUS status = STATUS_SUCCESS;
	
	
	
	DriverObject->MajorFunction[IRP_MJ_CREATE] = PassThru; //IRP_MJ_CREATE and IRP_MJ_CLOSE will operate the same way, 
														  //as I don't care about open/close state.
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = PassThru;
	DriverObject->MajorFunction[IRP_MJ_READ] = ReadKeyboardInput;
	DriverObject->DriverExtension->AddDevice = DeviceAttach;
	DriverObject->DriverUnload = latopUnload;
	
	
	
	return status;
}





void latopUnload(PDRIVER_OBJECT DriverObject) //FINISHED.
{	
	
	PDEVICE_OBJECT filter = DriverObject->DeviceObject; //my device object.
	while (filter != NULL) {// ill use a while loop to clear all the driver extensions that may be in the stack, just in case. 
		PDEVICE_EXTENSION devext = (PDEVICE_EXTENSION)(filter->DeviceExtension);
		if (devext->LowerDeviceObject != NULL) {//checks if the filter actually exists.

			IoDetachDevice(devext->LowerDeviceObject);
			;//deletes the filter device object that I used as reference to get to LowerDeviceObject, if and only if filter is not NULL.
			//IoDeleteDevice(DriverObject->DeviceObject) would've worked here as well. 

		}//this part is kind of shaky. had to improvise, not sure if it's safe or not..
		PDEVICE_OBJECT nextfilter = filter->NextDevice;//points to the next lowerdeviceobject in the stack. if there aren't any, filter = NULL and we break out of the loop.
		IoDeleteDevice(filter);
		filter = nextfilter;
	}
	
	 
	KdPrint(("Driver Unloaded\n"));
}

NTSTATUS PassThru(PDEVICE_OBJECT DeviceObject, PIRP irp) {//FINISHED
	UNREFERENCED_PARAMETER(DeviceObject);
	irp->IoStatus.Information = 0;//value of 0 is fine here.
	irp->IoStatus.Status = STATUS_SUCCESS;//expected status return.
	IoCompleteRequest(irp, IO_NO_INCREMENT); //Complete IRP request.
	return STATUS_SUCCESS;
}
NTSTATUS ReadKeyboardInput(PDEVICE_OBJECT DeviceObject, PIRP irp)
{
	//The order may be wrong. I think they should be under the IoSkipCurrentIrpStackLocation.
	UNREFERENCED_PARAMETER(DeviceObject);
	//initiates a contextinfo struct, which contains oldaffinity and oldpriority. this will be passed to "IoSetCompletionRoutine" as the "context" argument.
	PMY_COMPLETION_CONTEXT contextinfo = (PMY_COMPLETION_CONTEXT)ExAllocatePoolWithTag(NonPagedPool, sizeof(MY_COMPLETION_CONTEXT), 'CTXT');
	if (!contextinfo) { //checks if the allocation succeeded. 
		KdPrint(("Error: Failed to allocate memory\n"));
		return STATUS_INSUFFICIENT_RESOURCES;
	}
	PKTHREAD thread = KeGetCurrentThread(); //fetches the current thread

	contextinfo->oldprio = KeQueryPriorityThread(thread);//fetches current priority

	KeSetPriorityThread(thread, HIGH_PRIORITY); //sets the thread's priority to HIGH. this should speed up IRP processing.
	contextinfo->oldaffinity = KeSetSystemAffinityThreadEx(0x1); //Ensures code runs on CPU 0. This reduces context switching, which reduces latency.  

	NTSTATUS status = STATUS_SUCCESS;
	PDEVICE_EXTENSION deviceExtension = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension; //sets the device extension param that ill need in order to fetch the lowerdeviceobject from the stack.

	PIO_STACK_LOCATION stackLocation = IoGetCurrentIrpStackLocation(irp); //fetches the current stack location for the process. 
	
	//validate IRP and parameters, to ensure validity of inputs
	if (!irp || !stackLocation || !deviceExtension->LowerDeviceObject) {
		KdPrint(("Error: Invalid irp (0x%08X)\n", STATUS_INVALID_PARAMETER));
		//ExFreePoolWithTag(oldinfo, 'CTXT'); I'm unsure whether I should add this here or not.
		KeSetPriorityThread(thread, contextinfo->oldprio); //restores the old priority in case of failure.
		KeRevertToUserAffinityThreadEx(contextinfo->oldaffinity); //restores the old affinity in case of failure.
		return STATUS_INVALID_PARAMETER;

	}
	//skips my DeviceObject (filter) to go to LowerDeviceObject (DeviceObject of the physical keyboard).
	IoSkipCurrentIrpStackLocation(irp);

	IoSetCompletionRoutine(irp, CompletionRoutine, (PVOID)contextinfo, TRUE, TRUE, TRUE);//sets the completion routine to the one I defined. This is necessary, as the
	//completion routine is what implements the necessary modifications to reduce input lag. 
	//it also sends the oldprio as context, which is necessary in order to return the thread to it's regular priority. 

	status = IoCallDriver(deviceExtension->LowerDeviceObject, irp); //calls the driver for the keyboard with the IRP that 
	if (!NT_SUCCESS(status)) {
		irp->IoStatus.Status = status;
		irp->IoStatus.Information = 0;
		IoCompleteRequest(irp, IO_NO_INCREMENT);
		//ExFreePoolWithTag(oldinfo, 'CTXT'); I'm unsure whether I should add this here or not. 
		KeSetPriorityThread(thread, contextinfo->oldprio); //restores the old priority in case of failure.
		KeRevertToUserAffinityThreadEx(contextinfo->oldaffinity); //restores the old affinity in case of failure.
		KdPrint(("Error: invalid irp: (0x%08X)\n", status));
		return status;
	}

	
	
	
	return status;
}

NTSTATUS DeviceAttach(PDRIVER_OBJECT DriverObject, PDEVICE_OBJECT ActualKeyboard) //automatically called by the PNP Manager when it wants to add the device.
{
	NTSTATUS status = STATUS_SUCCESS; //initialize the status variable we will use.
	
	PDEVICE_OBJECT filter = NULL;
	PDEVICE_OBJECT LowerDeviceObject = NULL;
	UNICODE_STRING devname = RTL_CONSTANT_STRING(L"\\Device\\latop");
	status = IoCreateDevice(DriverObject, sizeof(DEVICE_EXTENSION), &devname, ActualKeyboard->DeviceType, 0, FALSE, &filter);
	if (!NT_SUCCESS(status)) {
		 
		KdPrint(("Failed to create device Object (0x%08X)\n", status));
		return status;
	}
	PDEVICE_EXTENSION deviceExtension = (PDEVICE_EXTENSION)filter->DeviceExtension;
	filter->Flags &= ~DO_DEVICE_INITIALIZING; //in order for DO_DEVICE_INITIALIZING not to clear all flags in order to initialize, we do a AND and a NOT on the flag that represents "DO_DEVICE_INITIALIZING" in order to clear it.
	filter->Flags |= DO_BUFFERED_IO;//DO_BUFFERED_IO:windows will copy data between usermode and a kernel buffer, making it easier to safely access user input. This helps reduce latency by minimizing the time spent on memory operations.
									//using OR bitwise operator here. this is because the filter Device may have flags that are already set.
									// we don't want to lose those, as they may- and probably are- crucial.
	status = IoAttachDeviceToDeviceStackSafe(filter, ActualKeyboard, &LowerDeviceObject);
	if (!NT_SUCCESS(status)) {
		IoDeleteDevice(filter);//this is in case it fails to attach itself to the DeviceStack of the keyboard. we still want to free the memory, so no leaks happen!
		KdPrint(("Failed to attach device Object (0x%08X)\n", status));
		//dont forget to delete symlink when I remember to initialize it lmao
		return status;
	}
	deviceExtension->LowerDeviceObject = LowerDeviceObject; //LowerDeviceObject is a necessary extension, as I may want to pass down IRPs in the stack.
	
	return status;
}

NTSTATUS CompletionRoutine(PDEVICE_OBJECT DeviceObject, PIRP irp, PVOID Context)
{

	UNREFERENCED_PARAMETER(DeviceObject);
	UNREFERENCED_PARAMETER(Context);
	
	KdPrint(("Completion Routine called. \n"));
	/*
	* this is where the magic happens.In order to lower inputlag, this completion routine will do 2 things, one active one passive.
	* 
	* 1. Temporary thread boosting(ACTIVE): This will be done by fetching the thread where the LowerDeviceObject resides, 
	* and temporarily increasing its priority to "HIGH". After processing the IRP request, the thread's priority will be lowered back 
	* down to "NORMAL", in order to avoid deadlocks, starvation, resource hogging, while improving stability and actually reducing input lag. 
	* 2. minimal context switches(Passive): Context switches occur when the CPU switches from executing one thread to another, which may 
	* significantly increase overhead. This is solved by using KeSetSystemAffinityThread, which forces the thread to remain on a specific CPU Core.
	* As such, there is less overhead, and therefore less input-lag.
	* 
	* TODO: Once I'm more skilled in driver development, I plan on implementing either DMA, IRP Batching(this syncs well with minimising context switches), 
	* or Kernel Bypassing techniques. 
	*/
	
	
	PKTHREAD thread = KeGetCurrentThread();//used to identify the thread handling the keyboard input. 
	PMY_COMPLETION_CONTEXT oldinfo = (PMY_COMPLETION_CONTEXT)Context; //restores the old prio 
	KeSetPriorityThread(thread, oldinfo->oldprio);//restores the old prio to the thread.
	KeRevertToUserAffinityThreadEx(oldinfo->oldaffinity);//restores the old affinity of the thread.
	ExFreePoolWithTag(oldinfo, 'CTXT');
	return STATUS_SUCCESS;
}
