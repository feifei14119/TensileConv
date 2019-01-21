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

        private void myInitialize()
        {
            IpAddr = "10.237.94.34";
            Username = "feifei";
            Password = "123";

            remoteWorkPath = "";
            remoteFolder = "_TensileConvTemp";
            tcExeFileName = "TensileConv.out";
            newLineRegex = new Regex(@"(\w\W)*@(\w*\W)*|:(\w\W)*\$ "); // [USER]@[HOST]:[PATH]$

            TbxIpAddr.Text = IpAddr;
            TbxUsername.Text = Username;
            TbxPassword.Text = Password;
            isConnected = false;

            BtnConnect.Content = "连接";
        }

        private bool isConnected = false;
        private SshClient ssh = null;
        private ScpClient scp = null;
        private ShellStream shell = null;
        private String IpAddr;
        private String Username;
        private String Password;

        private String remoteWorkPath;
        private String remoteFolder;
        private String tcExeFileName;
        private Regex newLineRegex;

        /// <summary>
        /// 连接或断开
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
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

                doCommand("mkdir " + remoteFolder);
                doCommand("cd " + remoteFolder);
                string result = doCommand("pwd");
                string[] strArray = result.Split(new string[] { "\r", "\n" }, StringSplitOptions.RemoveEmptyEntries);
                remoteWorkPath = strArray[1];


                //verifyEnvironment();

                uploadFile("./" + tcExeFileName);
                sudoCommand("chmod +x ./" + tcExeFileName);                  
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

                    Thread.Sleep(1000);
                    ssh = null;
                    shell = null;

                    BtnConnect.Content = "连接";
                }
                catch (Exception ex)
                {
                    BtnConnect.Content = "断开";
                    throw new Exception(string.Format("断开SSH失败，原因：{0}", ex.Message));
                }
            }
        }

        /// <summary>
        /// 直接执行命令
        /// </summary>
        /// <param name="cmd"></param>
        /// <returns></returns>
        private string doCommand(string cmd)
        {
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
                StringBuilder result = new StringBuilder();
                MatchCollection mcl;

                tmpRead = shell.Read();
                act = new Action(() =>
                {
                    TbxTest.AppendText(tmpRead);
                    TbxTest.ScrollToEnd();
                });
                this.Dispatcher.Invoke(act);
                shell.WriteLine(cmd);
                Thread.Sleep(200);

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

        /// <summary>
        /// sudo 执行命令
        /// </summary>
        /// <param name="cmd"></param>
        /// <returns></returns>
        private string sudoCommand(string cmd)
        {
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
                Thread.Sleep(200);

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
                        Thread.Sleep(1000);
                        continue;
                    }
                    if (result.ToString().EndsWith("密码： "))
                    {
                        shell.WriteLine(Password);
                        Thread.Sleep(1000);
                        continue;
                    }

                    // continue?
                    if (result.ToString().EndsWith("[Y/N] "))
                    {
                        shell.WriteLine("Y");
                        Thread.Sleep(1000);
                        continue;
                    }
                    if (result.ToString().EndsWith("[Y/n] "))
                    {
                        shell.WriteLine("Y");
                        Thread.Sleep(1000);
                        continue;
                    }
                    if (result.ToString().EndsWith("[y/N] "))
                    {
                        shell.WriteLine("y");
                        Thread.Sleep(1000);
                        continue;
                    }
                    if (result.ToString().EndsWith("[y/n] "))
                    {
                        shell.WriteLine("y");
                        Thread.Sleep(1000);
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

        private void uploadFile(string fileName)
        {
            scp = new ScpClient(IpAddr, Username, Password);

            try
            {
                scp.Connect();
                FileStream fsSrc = new FileStream(fileName, FileMode.Open);

                string remotPath = remoteWorkPath;
                string testttt = remoteWorkPath + "\\" + tcExeFileName;

                scp.Upload(fsSrc, remoteWorkPath + "/" + tcExeFileName);

                scp.Disconnect();
            }
            catch (Exception ex)
            {
                throw new Exception(string.Format("连接SFTP失败，原因：{0}", ex.Message));
            }
        }
    }
}
