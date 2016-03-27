using System.Threading;
using Microsoft.SPOT;
using GTM = Gadgeteer.Modules;
using uPLibrary.Networking.M2Mqtt;
using uPLibrary.Networking.M2Mqtt.Messages;
using System;
using System.Net;
using System.Net.Sockets;
using System.Text;
using GT = Gadgeteer;
using Gadgeteer.SocketInterfaces;

namespace CoffeeMaker
{
    public partial class Program
    {
        private const string DOMAIN = "Gadgeteer";
        private const string CLIENTID = "CoffeeMaker"; //this is the device id for the broker to use
        private const string DEVICEID = "device1";

        private const string COFFEEMAKERSTATUS = "status/CoffeMaker";
        private const string COFFEECONTROL = "cmd/Coffee";
        private const string DEVICESTATUS = "status/Device1";

        //servidor mqtt
        private const string BROKER = "m10.cloudmqtt.com";
        private const int PORT = 11001;
        private const string USERNAME = "test1";
        private const string PASSWORD = "test1";

        private MqttClient _mqttclient;

        private static bool _cleanSession = true;

        private readonly char[] _delimiters = { '/' }; //used to parse topic strings

        //only want control messages for this device (subscription)
        private const string mqttDeviceCommand = DOMAIN + "/" + DEVICEID + "/" + COFFEECONTROL + "/";

        // used to send out status messages for the lights (publish)
        private const string mqttCoffeStatus = DOMAIN + "/" + DEVICEID + "/" + COFFEEMAKERSTATUS + "/";

        // used to send out status messages for the device (publish)
        private const string mqttDeviceStatus = DOMAIN + "/" + DEVICEID + "/" + DEVICESTATUS;

        private readonly string[] DeviceSubscriptions = { mqttDeviceCommand };

        private readonly byte[] QOSServiceLevels = { 2 };

        GT.Timer timer = new GT.Timer(4000);
        GT.Timer Datos = new GT.Timer(20000);

        AnalogInput entrada = null;
        DigitalOutput salida = null;

        double Proximidad;

        // This method is run when the mainboard is powered up or reset.   
        void ProgramStarted()
        {
            ethernetJ11D.NetworkDown += EthernetJ11D_NetworkDown;
            ethernetJ11D.NetworkUp += EthernetJ11D_NetworkUp;
            ethernetJ11D.UseThisNetworkInterface();

            entrada = extender.CreateAnalogInput(GT.Socket.Pin.Three);
            salida = extender.CreateDigitalOutput(GT.Socket.Pin.Five, false);

            timer.Tick += Timer_Tick;
            Datos.Tick += Datos_Tick;

            Debug.Print("Program Started");
            setupEthernet();
            inicio();
            timer.Start();
        }

        private void Datos_Tick(GT.Timer timer)
        {
            salida.Write(true);
            Proximidad = entrada.ReadProportion();
        }

        private void Timer_Tick(GT.Timer timer)
        {
            try
            {
                if (_mqttclient.IsConnected)
                {

                }
                else
                {
                    byte response = _mqttclient.Connect(CLIENTID, USERNAME, PASSWORD, true, 2, true,
                            mqttDeviceStatus, "offline", _cleanSession, 60);

                    if (response == 0)
                    {
                        _mqttclient.Publish(mqttDeviceStatus, Encoding.UTF8.GetBytes("online"), 2, true);
                        Debug.Print(mqttDeviceStatus + " online : 2 true");
                    }
                    Debug.Print("Connect " + response);
                    timer.Start();
                    _mqttclient.Subscribe(DeviceSubscriptions, QOSServiceLevels);
                }
            }
            catch (Exception ex)
            {
                Debug.Print(ex.Message);
            }
        }

        private void inicio()
        {
            if (ethernetJ11D.IsNetworkConnected && (ethernetJ11D.NetworkInterface.IPAddress != "0.0.0.0"))
            {
                try
                {
                    NTPTime("time.windows.com", -360);
                    _mqttclient = new MqttClient(BROKER, PORT, false, null, null, MqttSslProtocols.None);
                    _mqttclient.MqttMsgPublishReceived += _mqttclient_MqttMsgPublishReceived;
                    _mqttclient.MqttMsgSubscribed += _mqttclient_MqttMsgSubscribed;
                    _mqttclient.MqttMsgPublished += _mqttclient_MqttMsgPublished;
                    _mqttclient.MqttMsgUnsubscribed += _mqttclient_MqttMsgUnsubscribed;

                }
                catch (Exception ex)
                {
                    Debug.Print(ex.Message);
                }
            }
        }

        private void ThreadedDisconnect()
        {
            _mqttclient.Disconnect();
        }

        private void _mqttclient_MqttMsgUnsubscribed(object sender, MqttMsgUnsubscribedEventArgs e)
        {
            Debug.Print("Msg Unsubscribed " + e.MessageId);
        }

        private void _mqttclient_MqttMsgPublished(object sender, MqttMsgPublishedEventArgs e)
        {
            Debug.Print("Msg Published " + e.MessageId);
        }

        private void _mqttclient_MqttMsgSubscribed(object sender, MqttMsgSubscribedEventArgs e)
        {
            Debug.Print("Message Subscribed " + e.MessageId);
        }

        private void _mqttclient_MqttMsgPublishReceived(object sender, MqttMsgPublishEventArgs e)
        {
            string[] topicparts = e.Topic.Split(_delimiters);
            Debug.Print(topicparts.Length.ToString());

            //get the message
            char[] chars = Encoding.UTF8.GetChars(e.Message);
            var message = new string(chars);

            Debug.Print(e.Topic + " " + message + " : " + e.QosLevel + " " + e.Retain + " " + e.DupFlag);

            if (topicparts.Length == 5 && topicparts[1] == DEVICEID && topicparts[2] == "cmd" &&
                topicparts[3] == "Coffee")
            {
                try
                {
                    //int light = int.Parse(topicparts[4]);
                    //SetLight(light, message);
                    if (message.Equals("on"))
                    {
                        multicolorLED.TurnBlue();
                        Debug.Print("se prende el foco");
                    }
                    else if (message.Equals("off"))
                    {
                        multicolorLED.TurnOff();
                        Debug.Print("se apaga el foco");
                    }
                }
                catch
                {
                    //MessageRecieved("Invalid Message " + e.Topic + " " + message + " : " + e.QosLevel + " " + e.Retain +
                    //                " " +
                    //                e.DupFlag);
                    Debug.Print("Invalid Message " + message);
                }
            }
        }

        public bool NTPTime(string TimeServer, int GmtOffset = 0)
        {
            Socket s = null;
            try
            {
                EndPoint rep = new IPEndPoint(Dns.GetHostEntry(TimeServer).AddressList[0], 123);
                s = new Socket(AddressFamily.InterNetwork, SocketType.Dgram, ProtocolType.Udp);
                var ntpData = new byte[48];
                Array.Clear(ntpData, 0, 48);
                ntpData[0] = 0x1B; // Set protocol version
                s.SendTo(ntpData, rep); // Send Request   
                if (s.Poll(30 * 1000 * 1000, SelectMode.SelectRead)) // Waiting an answer for 30s, if nothing: timeout
                {
                    s.ReceiveFrom(ntpData, ref rep); // Receive Time
                    byte offsetTransmitTime = 40;
                    ulong intpart = 0;
                    ulong fractpart = 0;
                    for (int i = 0; i <= 3; i++) intpart = (intpart << 8) | ntpData[offsetTransmitTime + i];
                    for (int i = 4; i <= 7; i++) fractpart = (fractpart << 8) | ntpData[offsetTransmitTime + i];
                    ulong milliseconds = (intpart * 1000 + (fractpart * 1000) / 0x100000000L);
                    s.Close();
                    DateTime dateTime = new DateTime(1900, 1, 1) +
                                        TimeSpan.FromTicks((long)milliseconds * TimeSpan.TicksPerMillisecond);
                    Microsoft.SPOT.Hardware.Utility.SetLocalTime(dateTime.AddMinutes(GmtOffset));
                    Debug.Print("Current Date and time " + DateTime.Now);
                    return true;
                }
                s.Close();
            }
            catch (Exception exception)
            {
                try
                {
                    s.Close();
                }
                catch
                {
                }
                Debug.Print(exception.Message);
            }
            return false;
        }

        private void setupEthernet()
        {
            ethernetJ11D.UseStaticIP(
                "192.168.100.10",
                "255.255.255.0",
                "192.168.100.1");
            string[] dns = { "192.168.100.1" };
            ethernetJ11D.NetworkSettings.EnableStaticDns(dns);

            while (!ethernetJ11D.IsNetworkUp)
            {
                Debug.Print("Waiting for NET");
                Thread.Sleep(250);
            }
        }

        private void EthernetJ11D_NetworkUp(GTM.Module.NetworkModule sender, GTM.Module.NetworkModule.NetworkState state)
        {
            Debug.Print("Network up.");
            ListNetworkInterfaces();
        }

        private void ListNetworkInterfaces()
        {
            Debug.Print("------------------------------------------------");
            Debug.Print("IP Address:   " + ethernetJ11D.NetworkSettings.IPAddress);
            Debug.Print("DHCP Enabled: " + ethernetJ11D.NetworkSettings.IsDhcpEnabled);
            Debug.Print("Subnet Mask:  " + ethernetJ11D.NetworkSettings.SubnetMask);
            Debug.Print("Gateway:      " + ethernetJ11D.NetworkSettings.GatewayAddress);
            Debug.Print("DNS:      " + ethernetJ11D.NetworkSettings.DnsAddresses.GetValue(0));
            Debug.Print("------------------------------------------------");
        }

        private void EthernetJ11D_NetworkDown(GTM.Module.NetworkModule sender, GTM.Module.NetworkModule.NetworkState state)
        {
            Debug.Print("Network down.");
        }
    }
}
