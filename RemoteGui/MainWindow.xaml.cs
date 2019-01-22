using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

using System.IO;
using Renci.SshNet;
using System.Threading;
using System.Text.RegularExpressions;

namespace TensileConvGui
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        public MainWindow()
        {
            InitializeComponent();
            myInitialize();
        }

        // 初始化
        private void myInitialize()
        {
            IpAddr = "10.237.94.34";
            Username = "feifei";
            Password = "123";

            remoteWorkPath = "";
            remoteFolder = "_TensileConvTemp";
            tcExeFileName = "TensileConv.out";

            TbxIpAddr.Text = IpAddr;
            TbxUsername.Text = Username;
            TbxPassword.Text = Password;
            isConnected = false;

            initControls();
        }

        private void initControls()
        {
            BtnConnect.Content = "连接";

            TbxTest.Text = "";

            tbxPlatName.Text = "";
            tbxPlatVen.Text = "";
            tbxPlatVer.Text = "";

            tbxDevName.Text = "";
            tbxDevVen.Text = "";
            tbxDevVer.Text = "";
            tbxDevCU.Text = "";
            tbxDevFreq.Text = "";
            tbxDevPerf.Text = "";
            lbxDevList.Items.Clear();
        }


        private bool isConnected = false;
        private SshClient ssh = null;
        private ScpClient scp = null;
        private ShellStream shell = null;
        private String IpAddr;
        private String Username;
        private String Password;

        private String remoteWorkPath;  // 远程路径
        private String remoteFolder;    // 创建的远程目录
        private String tcExeFileName;   // TensileConv可执行文件名称

        // SSH连接或断开
        private void BtnConnect_Click(object sender, RoutedEventArgs e)
        {
            IpAddr = TbxIpAddr.Text;
            Username = TbxUsername.Text;
            Password = TbxPassword.Text;

            if (ssh == null)
            {
                ssh = new SshClient(IpAddr, Username, Password);
            }

            if (!ssh.IsConnected)
            {
                // 连接
                try
                {
                    ssh.Connect();
                    shell = ssh.CreateShellStream("xtem", 180, 24, 800, 600, 1024);
                    BtnConnect.Content = "断开";
                    isConnected = true;
                }
                catch (Exception ex)
                {
                    isConnected = false;
                    BtnConnect.Content = "连接";
                    TbxTest.Text = "连接SSH失败，原因：{0}" + ex.Message;
                    throw new Exception(string.Format("连接SSH失败，原因：{0}", ex.Message));
                }

                //SshCommand terminal;
                //ssh.RunCommand("mkdir _temp_remote_rocm");
                //ssh.RunCommand("cd ./_temp_remote_rocm");
                //terminal = ssh.RunCommand("pwd");
                //remoteWorkPath = terminal.Result.Replace('\n', '/');

                if (isConnected == false)
                    return; 

                doCommand("mkdir " + remoteFolder);             // 建立临时目录
                doCommand("cd " + remoteFolder);                // 进入临时目录
                remoteWorkPath = doCommand("pwd");              // 获得临时目录全路径
                uploadFile("./" + tcExeFileName);               // 上传TensileConv.out文件
                sudoCommand("chmod +x ./" + tcExeFileName);     // 变为可执行文件
                doCommand("./TensileConv.out --evinfo 1");      // 获得硬件及运行时信息
            }
            else
            {
                // 断开连接
                try
                {
                    ssh.RunCommand("cd ..");
                    ssh.RunCommand("rm -rf " + remoteFolder);

                    shell.Close();
                    ssh.Disconnect();

                    Thread.Sleep(100);
                    ssh = null;
                    shell = null;

                    BtnConnect.Content = "连接";
                    initControls();
                }
                catch (Exception ex)
                {
                    BtnConnect.Content = "断开";
                    throw new Exception(string.Format("断开SSH失败，原因：{0}", ex.Message));
                }
            }
        }

        // 直接执行命令
        private string doCommand(string cmd)
        {
            if (isConnected != true)
                return null;

            Action act;
            String tmpRead;
            StringBuilder result = new StringBuilder();

            tmpRead = shell.Read();
            shell.WriteLine(cmd);

            while (true)
            {
                tmpRead = shell.ReadLine(new TimeSpan(0, 0, 1));
                if (tmpRead == null)
                    break;
                procEnvironmentInfo(tmpRead);

                result.Append(tmpRead);

                act = new Action(() =>
                {
                    TbxTest.AppendText(tmpRead);
                    TbxTest.AppendText("\n");
                    TbxTest.ScrollToEnd();
                });
                this.Dispatcher.Invoke(act);
            }

            if (result.Length > 0)
                result.Remove(0, cmd.Length);

            return result.ToString();
        }

        // sudo 执行命令
        private string sudoCommand(string cmd)
        {
            Regex newLineRegex = new Regex(@"(\w\W)*@(\w*\W)*|:(\w\W)*\$ "); // [USER]@[HOST]:[PATH]$

            if (ssh == null)
            {
                throw new Exception(string.Format("未连接"));
            }
            if (!ssh.IsConnected)
            {
                throw new Exception(string.Format("未连接"));
            }

            try
            {
                Action act;
                String tmpRead;
                MatchCollection mcl;
                StringBuilder result = new StringBuilder();

                shell.Read();
                shell.WriteLine("sudo " + cmd);

                //result.Append(shell.Read());
                //if(result.ToString().Contains("password for"))
                //{
                //    shell.WriteLine(Password);
                //}

                while (true)
                {
                    tmpRead = shell.Read();
                    result.Append(tmpRead);

                    act = new Action(() =>
                    {
                        TbxTest.AppendText(tmpRead);
                        TbxTest.ScrollToEnd();
                    });
                    this.Dispatcher.Invoke(act);

                    // password
                    if (result.ToString().EndsWith("password for " + Username + ": "))
                    {
                        shell.WriteLine(Password);
                        Thread.Sleep(500);
                        continue;
                    }
                    if (result.ToString().EndsWith("密码： "))
                    {
                        shell.WriteLine(Password);
                        Thread.Sleep(500);
                        continue;
                    }

                    // end
                    mcl = newLineRegex.Matches(result.ToString());
                    if (mcl.Count > 0)
                    {
                        break;
                    }

                    Thread.Sleep(100);
                }

                return result.ToString();
            }
            catch (Exception ex)
            {
                throw new Exception(string.Format("执行命令失败，原因：{0}", ex.Message));
            }
        }

        // 上传文件
        private void uploadFile(string fileName)
        {
            scp = new ScpClient(IpAddr, Username, Password);

            try
            {
                scp.Connect();
                FileStream fsSrc = new FileStream(fileName, FileMode.Open);                
                scp.Upload(fsSrc, remoteWorkPath + "/" + tcExeFileName);
                scp.Disconnect();
            }
            catch (Exception ex)
            {
                throw new Exception(string.Format("连接SFTP失败，原因：{0}", ex.Message));
            }
        }

        // 处理平台信息
        private bool platInfoStart = false;
        private bool devInfoStart = false;
        struct PlatInfo { public string platName, platVer, platVen; }
        struct DevInfo { public string devVen, devName, rtVer, cuNum, freq, perf; }
        private PlatInfo platInfo;
        private DevInfo devInfoTmp;
        private int devCnt = -1;
        private List<DevInfo> devInfo = new List<DevInfo>();
        private void procEnvironmentInfo(string str)
        {
            string[] tmpStrArr;
            Action act;

            if (str.Contains("*************************************************************************"))
            {
                if (platInfoStart == false)
                {
                    platInfoStart = true;
                    devInfo.Clear();
                }
                else
                {
                    platInfoStart = false;

                    act = new Action(() =>
                    {
                        tbxPlatName.Text = platInfo.platName;
                        tbxPlatVen.Text = platInfo.platVen;
                        tbxPlatVer.Text = platInfo.platVer;
                    });
                    this.Dispatcher.Invoke(act);
                }
            }

            if (platInfoStart)
            {
                if (str.Contains("Platform Name"))
                {
                    tmpStrArr = str.Split(new char[] { ':', '=' });
                    platInfo.platName = tmpStrArr.Last().Trim();
                }
                if (str.Contains("Version"))
                {
                    tmpStrArr = str.Split(new char[] { ':', '=' });
                    platInfo.platVer = tmpStrArr.Last().Trim();
                }
                if (str.Contains("Vendor Name"))
                {
                    tmpStrArr = str.Split(new char[] { ':', '=' });
                    platInfo.platVen = tmpStrArr.Last().Trim();
                }
            }

            if (str.Contains("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"))
            {
                if (devInfoStart == false)
                {
                    devInfoStart = true;
                    devInfoTmp = new DevInfo();
                    devCnt++;
                }
                else
                {
                    devInfoStart = false;
                    devInfo.Add(devInfoTmp);
                    act = new Action(() =>
                    {
                        lbxDevList.Items.Add("device: " + devCnt.ToString());
                        lbxDevList.SelectedIndex = 0;

                        tbxDevName.Text = devInfo[0].devName;
                        tbxDevVen.Text = devInfo[0].devVen;
                        tbxDevVer.Text = devInfo[0].rtVer;
                        tbxDevCU.Text = devInfo[0].cuNum;
                        tbxDevFreq.Text = devInfo[0].freq;
                        tbxDevPerf.Text = devInfo[0].perf;
                    });
                    this.Dispatcher.Invoke(act);
                }
            }

            if (devInfoStart)
            {
                if (str.Contains("Vendor Name"))
                {
                    tmpStrArr = str.Split(new char[] { ':', '=' });
                    devInfoTmp.devVen = tmpStrArr.Last().Trim();
                }
                if (str.Contains("Device Name"))
                {
                    tmpStrArr = str.Split(new char[] { ':', '=' });
                    devInfoTmp.devName = tmpStrArr.Last().Trim();
                }
                if (str.Contains("Runtime Version"))
                {
                    tmpStrArr = str.Split(new char[] { ':', '=' });
                    devInfoTmp.rtVer = tmpStrArr.Last().Trim();
                }
                if (str.Contains("CU Number"))
                {
                    tmpStrArr = str.Split(new char[] { ':', '=' });
                    devInfoTmp.cuNum = tmpStrArr.Last().Trim();
                }
                if (str.Contains("Core Frequency"))
                {
                    tmpStrArr = str.Split(new char[] { ':', '=' });
                    devInfoTmp.freq = tmpStrArr.Last().Trim();
                }
                if (str.Contains("Performance(fp32)"))
                {
                    tmpStrArr = str.Split(new char[] { ':', '=' });
                    devInfoTmp.perf = tmpStrArr.Last().Trim();
                }
            }
            
        }
    }
}
