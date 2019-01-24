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
            initControls();
        }

        // 初始化
        private void myInitialize()
        {
            ssh = null;
            shell = null;
            isConnected = false;
            IpAddr = "10.237.94.34";
            Username = "feifei";
            Password = "123";

            remoteWorkPath = "";
            remoteFolder = "_TensileConvTemp";
            tcExeFileName = "TensileConv.out";

            devCnt = -1;
            platInfo = new PlatInfo();
            allDevInfo = new List<DevInfo>();
            allDevInfo.Clear();

            N = 1;
            W = H = 14;
            C = 1024;
            K = 256;

        }

        private void initControls()
        {
            TbxIpAddr.Text = IpAddr;
            TbxUsername.Text = Username;
            TbxPassword.Text = Password;
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

            tbxN.Text = N.ToString();
            tbxC.Text = C.ToString();
            tbxH.Text = H.ToString();
            tbxW.Text = W.ToString();
            tbxK.Text = K.ToString();
            btnTensile.IsEnabled = false;
        }

        int N, C, H, W, K;
        private void BtnTensile_Click(object sender, RoutedEventArgs e)
        {
            N = int.Parse(tbxN.Text);
            C = int.Parse(tbxC.Text);
            H = int.Parse(tbxH.Text);
            W = int.Parse(tbxW.Text);
            K = int.Parse(tbxK.Text);

            if (isConnected == false)
                return;

            String cmd = "./TensileConv.out ";
            cmd += String.Format("-n {0:D} ", N);
            cmd += String.Format("-c {0:D} ", C);
            cmd += String.Format("-s {0:D} ", W);
            cmd += String.Format("-k {0:D} ", K);
            doWaitCmd(cmd);
        }


        private bool isConnected;
        private SshClient ssh;
        private ScpClient scp;
        private ShellStream shell;
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

            ssh = new SshClient(IpAddr, Username, Password);

            if (isConnected == false)
            {
                // 连接
                try
                {
                    ssh.Connect();
                    shell = ssh.CreateShellStream("xtem", 180, 24, 800, 600, 1024);
                    BtnConnect.Content = "断开";
                    btnTensile.IsEnabled = true;
                    isConnected = true;
                }
                catch (Exception ex)
                {
                    isConnected = false;
                    BtnConnect.Content = "连接";
                    TbxTest.Text = "连接SSH失败，原因：{0}" + ex.Message;
                }

                if (isConnected == false)
                    return;

                doImdtCmd("mkdir " + remoteFolder);        // 建立临时目录
                doImdtCmd("cd " + remoteFolder);           // 进入临时目录
                remoteWorkPath = doImdtCmd("pwd", 2000000);              // 获得临时目录全路径
                uploadFile("./" + tcExeFileName);               // 上传TensileConv.out文件
                sudoImdtCmd("chmod +x ./" + tcExeFileName);     // 变为可执行文件

                doImdtCmd("./TensileConv.out --evinfo 1", 5000000, procEnvironmentInfo);      // 获得硬件及运行时信息
              //  doWaitCmd("./TensileConv.out --evinfo 1", procEnvironmentInfo);      // 获得硬件及运行时信息
            }
            else // 断开连接
            {               
                doImdtCmd("cd ..");
                doImdtCmd("rm -rf " + remoteFolder);

                shell.Close();
                ssh.Disconnect();
                Thread.Sleep(100);

                myInitialize();
                initControls();
            }
        }

        // 直接执行命令
        MatchCollection mcl;
        Regex newLineRegex = new Regex(@"(\w\W)*@(\w*\W)*|:(\w\W)*\$ ");    // [USER]@[HOST]:[PATH]$
        private delegate void procCmdRsltDlg(string s);
        private string doImdtCmd(string cmd, long timeOut = 1000000, procCmdRsltDlg procCmdRslt = null) // 100ms
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
                tmpRead = shell.ReadLine(new TimeSpan(timeOut));    

                if (tmpRead == null)
                    break;

                if(procCmdRslt != null)
                    procCmdRslt(tmpRead);
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
        private string doWaitCmd(string cmd, procCmdRsltDlg procCmdRslt = null)
        {
            if (isConnected != true)
                return null;

            Action act;
            String tmpRead;
            String newLineTmp = "";
            String[] tmpStr;
            StringBuilder result = new StringBuilder();

            tmpRead = shell.Read();
            shell.WriteLine(cmd);
            Thread.Sleep(10);

            while (true)
            {
                tmpRead = shell.Read();
                result.Append(tmpRead);
                tmpStr = tmpRead.Split('\n');
                if (tmpStr.Length > 1)
                {
                    if (procCmdRslt != null)
                        procCmdRslt(newLineTmp + tmpStr[0]);
                    for (int i = 0; i < tmpStr.Length - 1; i++)
                    {
                        if (procCmdRslt != null)
                            procCmdRslt(tmpStr[i]);
                    }
                    newLineTmp = tmpStr[tmpStr.Length - 1];
                }
                else
                {
                    newLineTmp += tmpStr[0];
                }

                act = new Action(() =>
                {
                    TbxTest.AppendText(tmpRead);
                    TbxTest.ScrollToEnd();
                });
                this.Dispatcher.Invoke(act);

                mcl = newLineRegex.Matches(result.ToString());
                if (mcl.Count > 0)
                    break;

                Thread.Sleep(100);
            }

            return result.ToString();
        }
        private string sudoImdtCmd(string cmd, long timeOut = 1000000, procCmdRsltDlg procCmdRslt = null)
        {
            if (isConnected != true)
                return null;

            Action act;
            String tmpRead;
            StringBuilder result = new StringBuilder();

            shell.Read();
            shell.WriteLine("sudo " + cmd);
            Thread.Sleep(200);
            shell.WriteLine(Password);

            while (true)
            {
                tmpRead = shell.ReadLine(new TimeSpan(timeOut));

                if (tmpRead == null)
                    break;

                if (procCmdRslt != null)
                    procCmdRslt(tmpRead);
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
        private string sudoWaitCmd(string cmd, procCmdRsltDlg procCmdRslt = null)
        {
            if (isConnected != true)
                return null;

            try
            {
                Action act;
                String tmpRead;
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
                        TbxTest.AppendText("\n");
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
        private int devCnt;
        private List<DevInfo> allDevInfo;
        private void procEnvironmentInfo(string str)
        {
            string[] tmpStrArr;
            Action act;

            if (str.Contains("*************************************************************************"))
            {
                if (platInfoStart == false)
                {
                    platInfoStart = true;
                    platInfo = new PlatInfo();

                    allDevInfo = new List<DevInfo>();
                    allDevInfo.Clear();
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
                    allDevInfo.Add(devInfoTmp);
                    act = new Action(() =>
                    {
                        lbxDevList.Items.Add("device: " + devCnt.ToString());
                        lbxDevList.SelectedIndex = 0;

                        tbxDevName.Text = allDevInfo[0].devName;
                        tbxDevVen.Text = allDevInfo[0].devVen;
                        tbxDevVer.Text = allDevInfo[0].rtVer;
                        tbxDevCU.Text = allDevInfo[0].cuNum;
                        tbxDevFreq.Text = allDevInfo[0].freq;
                        tbxDevPerf.Text = allDevInfo[0].perf;
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
