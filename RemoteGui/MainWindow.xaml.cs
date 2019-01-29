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

            remoteWorkPath = "";
            remoteFolder = "_TensileConvTemp";
            tcExeFileName = "TensileConv.out";

            devCnt = -1;
            platInfo = new PlatInfo();
            allDevInfo = new List<DevInfo>();
            allDevInfo.Clear();

            N = 1;
            W = H = 14;
            C = 1024; K = 256;
            OutW = OutH = OutK = OutN = 0;
            InputSize = WeightSize = OutputSize = "0MB";
            Calculation = TheoryElapsedTime = "0";
            initInfoFlag = true; kernelParamFlag = false; finishFlag = false;
            KernelIterations = 100;
            ElapsedTime = Performence = Efficiency = "0";
            BestElapsedTime = BestPerformence = BestEfficiency = "0";
            SearchingPercent = 0;
            SearchedKernel = TotalKernel = 0;
            TunnedProblem = TotalProblem = 1;
            pck_order = 123;
            c_lds_atomic = c_lds_split = c_l2_atomic = c_l2_split = 1;
            k_out_maps = 8;
            group_size_x = 64; group_size_y = 1;
            global_size_x = 1; global_size_y = 1;
            sig_slot_size = l2_tmp_size = "0MB";
            mean_err = 0;
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
            tbxOutW.Text = OutW.ToString();
            tbxOutH.Text = OutH.ToString();
            tbxGroupSz.Text = String.Format("{0:D}, {1:D}, 1", group_size_x, group_size_y);
            tbxGlobalSz.Text = String.Format("{0:D}, {1:D}, 1", global_size_x, global_size_y);
            tbxPck.Text = pck_order.ToString();
            tbxLdsAtomic.Text = c_lds_atomic.ToString();
            tbxLdsSplit.Text = c_lds_split.ToString();
            tbxL2Atomic.Text = c_l2_atomic.ToString();
            tbxL2Split.Text = c_l2_split.ToString();
            tbxKoutMaps.Text = k_out_maps.ToString();
            tbxInSize.Text = InputSize.ToString();
            tbxWeiSize.Text = WeightSize.ToString();
            tbxOutSize.Text = OutputSize.ToString();
            tbxSigSize.Text = sig_slot_size.ToString();
            tbxL2Size.Text = l2_tmp_size.ToString();
            tbxElapsed.Text = BestElapsedTime.ToString();
            tbxPerf.Text = BestPerformence.ToString();
            tbxEffic.Text = BestEfficiency.ToString();
            tbxCalcu.Text = Calculation.ToString();
            tbxtheore.Text = TheoryElapsedTime.ToString();

            tbxProcess.Text = String.Format("Tunning Problem: {0:D}/{1:D}... Searching Kernel: {2:D}/{3:D}...",
                TunnedProblem, TotalProblem, SearchedKernel, TotalProblem);
            tbxPercent.Text = String.Format("{0:F2}%", SearchingPercent * 100);
            btnTensile.IsEnabled = false;
        }

        int N, C, H, W, K;
        private void BtnTensile_Click(object sender, RoutedEventArgs e)
        {
            TbxTest.Text = "";
            N = int.Parse(tbxN.Text);
            C = int.Parse(tbxC.Text);
            H = int.Parse(tbxH.Text);
            W = int.Parse(tbxW.Text);
            K = int.Parse(tbxK.Text);

            if (isConnected == false)
                return;

            Thread tc = new Thread(TensileConv);
            tc.Start();
        }
        private void TensileConv()
        {
            String cmd = "./TensileConv.out ";
            cmd += String.Format("-n {0:D} ", N);
            cmd += String.Format("-c {0:D} ", C);
            cmd += String.Format("-s {0:D} ", W);
            cmd += String.Format("-k {0:D} ", K);
            initInfoFlag = true;
            kernelParamFlag = false;
            finishFlag = false;
            doWaitCmd(cmd, procTensileLog);
        }
        
        private bool isConnected;
        private SshClient ssh;
        private ScpClient scp;
        private ShellStream shell;
        //private String IpAddr = "10.237.94.34";
        //private String Username = "feifei";
        //private String Password = "123";
        //private String IpAddr = "10.67.10.126";
        //private String Username = "dladmin";
        //private String Password = "123456";
        private String IpAddr = "10.237.94.93";
        private String Username = "root";
        private String Password = "123456";

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

                if (Username == "root")
                {
                    doImdtCmd("cd /home");
                }
                doImdtCmd("mkdir " + remoteFolder);        // 建立临时目录
                doImdtCmd("cd " + remoteFolder);           // 进入临时目录
                remoteWorkPath = doImdtCmd("pwd");              // 获得临时目录全路径
                uploadFile("./" + tcExeFileName);               // 上传TensileConv.out文件
                sudoImdtCmd("chmod +x ./" + tcExeFileName);     // 变为可执行文件
                Thread.Sleep(200);
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
        private delegate void procCmdRsltDlg(String s);
        private String doImdtCmd(String cmd, long timeOut = 2000000, procCmdRsltDlg procCmdRslt = null) // 100ms
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
        private String doWaitCmd(String cmd, procCmdRsltDlg procCmdRslt = null)
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
            //    result.Append(tmpRead);
                tmpStr = tmpRead.Split('\n');
                if (tmpStr.Length > 1)
                {
                    if (procCmdRslt != null)
                    {
                        procCmdRslt(newLineTmp + tmpStr[0]);
                    }
                    mcl = newLineRegex.Matches(newLineTmp + tmpStr[0]);
                    if (mcl.Count > 0)
                        break;

                    for (int i = 0; i < tmpStr.Length - 1; i++)
                    {
                        if (procCmdRslt != null)
                        {
                            procCmdRslt(tmpStr[i]);
                        }
                        mcl = newLineRegex.Matches(tmpStr[i]);
                        if (mcl.Count > 0)
                            break;
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

                Thread.Sleep(1);
            }

            return result.ToString();
        }
        private String sudoImdtCmd(String cmd, long timeOut = 2000000, procCmdRsltDlg procCmdRslt = null)
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
        private String sudoWaitCmd(String cmd, procCmdRsltDlg procCmdRslt = null)
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
                throw new Exception(String.Format("执行命令失败，原因：{0}", ex.Message));
            }
        }

        // 上传文件
        private void uploadFile(String fileName)
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
                throw new Exception(String.Format("连接SFTP失败，原因：{0}", ex.Message));
            }
        }

        // 处理平台信息
        private bool platInfoStart = false;
        private bool devInfoStart = false;
        struct PlatInfo { public String platName, platVer, platVen; }
        struct DevInfo { public String devVen, devName, rtVer, cuNum, freq, perf; }
        private PlatInfo platInfo;
        
        private DevInfo devInfoTmp;
        private int devCnt;
        private List<DevInfo> allDevInfo;
        private void procEnvironmentInfo(String str)
        {
            String[] tmpStrArr;
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

        int OutW, OutH, OutK, OutN;
        String InputSize, WeightSize, OutputSize;
        String Calculation, TheoryElapsedTime;
        bool initInfoFlag, kernelParamFlag, finishFlag;
        int KernelIterations;
        String ElapsedTime, Performence, Efficiency;
        String BestElapsedTime, BestPerformence, BestEfficiency;
        double SearchingPercent;
        int SearchedKernel, TotalKernel;
        int TunnedProblem, TotalProblem;
        int pck_order, c_lds_atomic, c_lds_split, c_l2_atomic, c_l2_split, k_out_maps;
        int group_size_x, group_size_y, global_size_x, global_size_y;
        String sig_slot_size, l2_tmp_size;
        double mean_err;
        private void procTensileLog(String logStr)
        {
            Action act;
            String tmpLog;
            tmpLog = delLogHead(logStr);

            if (initInfoFlag == true)
            {
                if (tmpLog.StartsWith("output WHKN"))
                {
                    tmpLog = tmpLog.Split('=')[1];
                    tmpLog = tmpLog.Trim();
                    OutW = int.Parse(tmpLog.Split(',')[0].Trim());
                    OutH = int.Parse(tmpLog.Split(',')[1].Trim());
                    OutK = int.Parse(tmpLog.Split(',')[2].Trim());
                    OutN = int.Parse(tmpLog.Split(',')[3].Trim());
                }
                if (tmpLog.StartsWith("init tensor input"))
                {
                    InputSize = tmpLog.Split('=')[2].Trim();
                }
                if (tmpLog.StartsWith("init tensor weight"))
                {
                    WeightSize = tmpLog.Split('=')[2].Trim();
                }
                if (tmpLog.StartsWith("init tensor output"))
                {
                    OutputSize = tmpLog.Split('=')[2].Trim();
                }
                if (tmpLog.StartsWith("Calculation"))
                {
                    Calculation = tmpLog.Split(',')[0].Trim().Split('=')[1].Trim();
                    TheoryElapsedTime = tmpLog.Split(',')[1].Trim().Split('=')[1].Trim();
                }
                if (tmpLog.StartsWith("run host calculate"))
                {
                    initInfoFlag = false;
                    kernelParamFlag = false;

                    act = new Action(() =>
                    {
                        tbxOutW.Text = OutW.ToString();
                        tbxOutH.Text = OutH.ToString();
                        tbxInSize.Text = InputSize.ToString();
                        tbxWeiSize.Text = WeightSize.ToString();
                        tbxOutSize.Text = OutputSize.ToString();
                        tbxCalcu.Text = Calculation.ToString();
                        tbxtheore.Text = TheoryElapsedTime.ToString();
                    });
                    this.Dispatcher.Invoke(act);
                }
            }

            if (kernelParamFlag == true)
            {
                if (tmpLog.Contains("PCK_order"))
                {
                    pck_order = int.Parse(getNumStr(tmpLog.Split('=')[1].Trim()));
                }
                if (tmpLog.Contains("c_lds_atomic") && tmpLog.Contains("c_lds_split"))
                {
                    c_lds_atomic = int.Parse(tmpLog.Split(',')[0].Split('=')[1].Trim());
                    c_lds_split = int.Parse(tmpLog.Split(',')[1].Split('=')[1].Trim());
                }
                if (tmpLog.Contains("c_l2_atomic") && tmpLog.Contains("c_l2_split"))
                {
                    c_l2_atomic = int.Parse(tmpLog.Split(',')[0].Split('=')[1].Trim());
                    c_l2_split = int.Parse(tmpLog.Split(',')[1].Split('=')[1].Trim());
                }
                if (tmpLog.Contains("k_out_maps") && tmpLog.Contains("k_out_group"))
                {
                    k_out_maps = int.Parse(tmpLog.Split(',')[0].Split('=')[1].Trim());
                }
                if (tmpLog.Contains("group_size"))
                {
                    group_size_x = int.Parse(tmpLog.Split('=')[1].Split(',')[0].Trim());
                    group_size_y = int.Parse(tmpLog.Split('=')[1].Split(',')[1].Trim());
                }
                if (tmpLog.Contains("sigal_size") && tmpLog.Contains("l2_size"))
                {
                    sig_slot_size = tmpLog.Split(',')[0].Split('=')[1].Trim();
                    l2_tmp_size = tmpLog.Split(',')[1].Split('=')[1].Trim();
                }
                if (tmpLog.Contains("-------------------------------------------------------------------------"))
                {
                    kernelParamFlag = false;

                    act = new Action(() =>
                    {
                        tbxPck.Text = pck_order.ToString();
                        tbxLdsAtomic.Text = c_lds_atomic.ToString();
                        tbxLdsSplit.Text = c_lds_split.ToString();
                        tbxL2Atomic.Text = c_l2_atomic.ToString();
                        tbxL2Split.Text = c_l2_split.ToString();
                        tbxKoutMaps.Text = k_out_maps.ToString();
                        tbxSigSize.Text = sig_slot_size.ToString();
                        tbxL2Size.Text = l2_tmp_size.ToString();
                        tbxGroupSz.Text = String.Format("{0:D}, {1:D}, 1", group_size_x, group_size_y);
                    });
                    this.Dispatcher.Invoke(act);
                }
            }
            else
            {
                if (tmpLog.StartsWith("launch kernel"))
                {
                    KernelIterations = int.Parse(getNumStr(tmpLog));
                }
                if (tmpLog.Contains("elapsed") && tmpLog.Contains("performence") && (!tmpLog.StartsWith("Best for now")))
                {
                    ElapsedTime = tmpLog.Split(',')[0].Split('=')[1].Trim();
                    Performence = tmpLog.Split(',')[1].Split('=')[1].Trim();
                    Efficiency = tmpLog.Split(',')[1].Split('=')[2].Trim();
                }
                if (tmpLog.StartsWith("Best for now:"))
                {
                    BestElapsedTime = tmpLog.Split(',')[0].Split('=')[1].Trim();
                    BestEfficiency = tmpLog.Split(',')[1].Split('=')[1].Trim();
                }
                if (tmpLog.StartsWith("Searching"))
                {
                    SearchedKernel = int.Parse(getNumStr(tmpLog.Split(':')[1]).Split('/')[0].Trim());
                    TotalKernel = int.Parse(getNumStr(tmpLog.Split(':')[1]).Split('/')[1].Trim());
                    SearchingPercent = 1.0 * SearchedKernel / TotalKernel;
                }
                if (tmpLog.Contains("Kernel Param:"))
                {
                    kernelParamFlag = true;

                    act = new Action(() =>
                    {
                        tbxElapsed.Text = BestElapsedTime.ToString();
                        tbxPerf.Text = BestPerformence.ToString();
                        tbxEffic.Text = BestEfficiency.ToString();
                        tbxProcess.Text = String.Format("Tunning Problem: {0:D}/{1:D}... Searching Kernel: {2:D}/{3:D}...",
                            TunnedProblem, TotalProblem, SearchedKernel, TotalKernel);
                        tbxPercent.Text = String.Format("{0:F2}%", SearchingPercent * 100);
                        pbProcPercent.Value = SearchingPercent * 100;
                    });
                    this.Dispatcher.Invoke(act);
                }
                if (tmpLog.Contains("search kernel parameters finished"))
                {
                    finishFlag = true;
                }
            }

            if(finishFlag == true)
            {
                if(tmpLog.Contains("Best score"))
                {
                    BestElapsedTime = tmpLog.Split(':')[1].Split(',')[0].Trim();
                    BestPerformence = tmpLog.Split(':')[1].Split(',')[1].Trim();
                    BestEfficiency = tmpLog.Split(':')[1].Split(',')[2].Trim();
                }
                if (tmpLog.Contains("+ group_size ="))
                {
                    group_size_x = int.Parse(tmpLog.Split('=')[1].Split(',')[0].Trim());
                    group_size_y = int.Parse(tmpLog.Split('=')[1].Split(',')[1].Trim());
                }
                if (tmpLog.Contains("global_size"))
                {
                    global_size_x = int.Parse(tmpLog.Split('=')[1].Split(',')[0].Trim());
                    global_size_y = int.Parse(tmpLog.Split('=')[1].Split(',')[1].Trim());
                }
                if (tmpLog.Contains("PCK_order"))
                {
                    pck_order = int.Parse(tmpLog.Split('=')[1].Trim());
                }
                if (tmpLog.Contains("c_in_lds_atomic_group"))
                {
                    c_lds_atomic = int.Parse(tmpLog.Split('=')[1].Trim());
                }
                if (tmpLog.Contains("c_in_lds_split_group"))
                {
                    c_lds_split = int.Parse(tmpLog.Split('=')[1].Trim());
                }
                if (tmpLog.Contains("c_in_l2_atomic_group"))
                {
                    c_l2_atomic = int.Parse(tmpLog.Split('=')[1].Trim());
                }
                if (tmpLog.Contains("c_in_l2_split_group"))
                {
                    c_l2_split = int.Parse(tmpLog.Split('=')[1].Trim());
                }
                if (tmpLog.Contains("k_out_maps")&&(!tmpLog.Contains("k_out_group")))
                {
                    k_out_maps = int.Parse(tmpLog.Split('=')[1].Trim());
                }
                if (tmpLog.Contains("mean err"))
                {
                    if (tmpLog.Contains('@'))
                    {
                        mean_err = double.Parse(getNumStr(tmpLog.Split('=')[1].Split('@')[1]));
                    }
                    else
                    {
                        mean_err = double.Parse(tmpLog.Split('=')[1].Trim());
                    }
                }
                if(tmpLog.Contains("release host"))
                {
                    act = new Action(() =>
                    {
                        tbxElapsed.Text = BestElapsedTime.ToString();
                        tbxPerf.Text = BestPerformence.ToString();
                        tbxEffic.Text = BestEfficiency.ToString();
                        tbxPck.Text = pck_order.ToString();
                        tbxLdsAtomic.Text = c_lds_atomic.ToString();
                        tbxLdsSplit.Text = c_lds_split.ToString();
                        tbxL2Atomic.Text = c_l2_atomic.ToString();
                        tbxL2Split.Text = c_l2_split.ToString();
                        tbxKoutMaps.Text = k_out_maps.ToString();
                        tbxSigSize.Text = sig_slot_size.ToString();
                        tbxL2Size.Text = l2_tmp_size.ToString();
                        SearchedKernel = TotalKernel;
                        tbxGroupSz.Text = String.Format("{0:D}, {1:D}, 1", group_size_x, group_size_y);
                        tbxGlobalSz.Text = String.Format("{0:D}, {1:D}, 1", global_size_x, global_size_y);
                        tbxProcess.Text = String.Format("Tunning Problem: {0:D}/{1:D}... Searching Kernel: {2:D}/{3:D}...",
                            TunnedProblem, TotalProblem, SearchedKernel, TotalKernel);
                        tbxPercent.Text = String.Format("{0:F2}%", SearchingPercent * 100);
                        pbProcPercent.Value = 100;
                    });
                    this.Dispatcher.Invoke(act);
                }
            }
        }

        private String delLogHead(String logStr)
        {
            if (!logStr.StartsWith("[INFO]"))
                return logStr;

            logStr = logStr.Remove(0, logStr.IndexOf(']') + 1);
            logStr = logStr.Remove(0, logStr.IndexOf(']') + 1);

            return logStr;
        }
        char[] IntChar = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9' };
        private String getNumStr(String logStr)
        {
            logStr = logStr.Remove(0, logStr.IndexOfAny(IntChar));
            logStr = logStr.Remove(logStr.LastIndexOfAny(IntChar)+1, logStr.Length- logStr.LastIndexOfAny(IntChar)-1);
            return logStr;
        }
    }
}
