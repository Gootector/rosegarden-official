


#ifndef DEVICESMANAGERNEW_H
#define DEVICESMANAGERNEW_H


#include "gui/ui/DevicesManagerNewUi.h"
#include <QWidget>
#include <QDialog>
#include <QObject>

#include "base/Device.h"
#include "base/MidiDevice.h"
#include "base/MidiTypes.h"
#include "base/Studio.h"


namespace Rosegarden
{

	typedef std::vector<MidiDevice *> MidiDeviceList;
	
	class RosegardenGUIDoc;
	class Studio;
	
	class DevicesManagerNew : public QDialog, public Ui::DevicesManagerNewUi
	{
		Q_OBJECT
		
		public:
			
			DevicesManagerNew ( QWidget* parent, RosegardenGUIDoc* doc );
			~DevicesManagerNew();
			
			
			/**
			*	Clear all lists
			*/
			void clearAllPortsLists( );
			
			/**
			*	make Slot connections
			*/
			void connectSignalsToSlots( );
			
			MidiDevice* getDeviceByName( QString deviceName );
			MidiDevice* getDeviceById( DeviceId devId );
			
			MidiDevice* getCurrentlySelectedDevice( QTreeWidget* treeWid );
			
			void connectMidiDeviceToPort ( MidiDevice* mdev, QString portName );
			
			/**
			*	If the selected device has changed, this
			*	markes (checkes) the associated list entry in the ports list (connection)
			*/
			void updateCheckStatesOfPortsList( QTreeWidget* treeWid_ports, QTreeWidget* treeWid_devices );
			
			/**
			*	adds/removes list entries in the visible devices-list (treeWid),
			*	if the (invisible) device-list of the sequencer have changed
			*/
			void updateDevicesList( DeviceList* devices, QTreeWidget* treeWid, 
									MidiDevice::DeviceDirection in_out_direction );
			
			/**
			*	search treeWid for the item associated with devId
			*/
			QTreeWidgetItem* searchItemWithDeviceId( QTreeWidget* treeWid, DeviceId devId );
			
			QTreeWidgetItem* searchItemWithPort( QTreeWidget* treeWid, QString portName );
			
			/**
			*	add/remove list entries in the visible ports-list (connections),
			*	if the (invisible) connections of the sequencer/studio have changed.
			*/
			void updatePortsList( QTreeWidget* treeWid, MidiDevice::DeviceDirection PlayRecDir );
			
			
		// START SIGNALS
		// ---------------------------------------------------------
		signals:
			//void deviceNamesChanged();
			
			void editBanks ( DeviceId );
			void editControllers ( DeviceId );
			
			
		// START SLOTS
		// ---------------------------------------------------------
		public slots:
			void slotOutputPortDoubleClicked( QTreeWidgetItem * item, int column );
			void slotOutputPortClicked( QTreeWidgetItem * item, int column );
			void slotPlaybackDeviceSelected();
			
			void slotInputPortDoubleClicked( QTreeWidgetItem * item, int column );
			void slotInputPortClicked( QTreeWidgetItem * item, int column );
			void slotRecordDeviceSelected();
			
			void slotRefreshOutputPorts();
			void slotRefreshInputPorts();
			
			void slotAddPlaybackDevice();
			void slotAddRecordDevice();
			
			void slotDeletePlaybackDevice();
			void slotDeleteRecordDevice();
			
			void slotManageBanksOfPlaybackDevice();
			void slotEditControllerDefinitions();
			
			void slotAddLV2Device();
			
			void show();
			void slotClose();
			void slotHelpRequested();
			
		// PROTECTED
		// ---------------------------------------------------------
		protected:
			//
			RosegardenGUIDoc *m_doc;
			Studio *m_studio;
			
			/**
			*	used to store the device ID in the QTreeWidgetItem
			*	of the visible device list (QTreeWidget)
			*/
			int m_UserRole_DeviceId; // = Qt::UserRole + 1;
			
// 			MidiDeviceList m_playDevices;
// 			MidiDeviceList m_recordDevices;
			
	};


} // end namespace Rosegarden

#endif // DEVICESMANAGERNEW_H

