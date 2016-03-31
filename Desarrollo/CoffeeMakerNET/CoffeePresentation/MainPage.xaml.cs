using System.Text;
using uPLibrary.Networking.M2Mqtt;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;

// The Blank Page item template is documented at http://go.microsoft.com/fwlink/?LinkId=402352&clcid=0x409

namespace CoffeePresentation
{
    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class MainPage : Page
    {
        private const string DOMAIN = "Gadgeteer";
        private const string CLIENTID = "CoffeeMaker"; //this is the device id for the broker to use
        private const string DEVICEID = "device1";

        private const string COFFEECONTROL = "cmd/Coffee";
        private const string DEVICESTATUS = "Status/W10Satus";

        private const string BROKER = "m10.cloudmqtt.com";
        private const int PORT = 21001;
        private const string USERNAME = "test1";
        private const string PASSWORD = "test1";

        private MqttClient _mqttclient;

        private readonly char[] _delimiters = { '/' }; //used to parse topic strings

        //only want control messages for this device (subscription)
        private const string mqttDeviceCommand = DOMAIN + "/" + DEVICEID + "/" + COFFEECONTROL + "/";
        private const string mqttDeviceStatus = DOMAIN + "/" + DEVICEID + "/" + DEVICESTATUS;

        private bool _cleanSession = true;

        public MainPage()
        {
            InitializeComponent();
            _mqttclient = new MqttClient(BROKER, PORT, false, MqttSslProtocols.SSLv3);
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
                }
            }
        }

        private void OnButton_Click(object sender, RoutedEventArgs e)
        {
            _mqttclient.Connect(CLIENTID, USERNAME, PASSWORD);
            _mqttclient.Publish(mqttDeviceCommand, Encoding.UTF8.GetBytes("on"), 2, true);
        }

        private void OffButton_Click(object sender, RoutedEventArgs e)
        {
            _mqttclient.Connect(CLIENTID, USERNAME, PASSWORD);
            _mqttclient.Publish(mqttDeviceCommand, Encoding.UTF8.GetBytes("off"), 2, true);
        }
    }
}
