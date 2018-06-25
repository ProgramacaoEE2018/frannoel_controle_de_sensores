using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.IO.Ports;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace WindowsFormsApp1
{
	public partial class Form1 : Form
	{
		Thread X;
		public Form1()
		{
			InitializeComponent();
			ComboPorta.DataSource = SerialPort.GetPortNames();
			X = new Thread(new ThreadStart(Serial));
		}

		private void atualizaListaCOMs()
		{
			int i;
			bool quantDiferente;    //flag para sinalizar que a quantidade de portas mudou

			i = 0;
			quantDiferente = false;

			//se a quantidade de portas mudou
			if (ComboPorta.Items.Count == SerialPort.GetPortNames().Length)
			{
				foreach (string s in SerialPort.GetPortNames())
				{
					if (ComboPorta.Items[i++].Equals(s) == false)
					{
						quantDiferente = true;
					}
				}
			}
			else
			{
				quantDiferente = true;
			}

			//Se não foi detectado diferença
			if (quantDiferente == false)
			{
				return;                     //retorna
			}

			//limpa comboBox
			ComboPorta.Items.Clear();

			//adiciona todas as COM diponíveis na lista
			foreach (string s in SerialPort.GetPortNames())
			{
				ComboPorta.Items.Add(s);
			}
			//seleciona a primeira posição da lista
			ComboPorta.SelectedIndex = 0;
		}
		private void Button1_Click(object sender, EventArgs e)
		{
			if (SP.IsOpen)
			{
				SP.Close();
				X.Abort();
				ComboPorta.Enabled = true;
				timerx.Enabled = false;
				
			}
			else
			{
				SP.PortName = ComboPorta.Text;
				SP.BaudRate = Convert.ToInt32(ComboBaud.Text);
				SP.Open();
				//X.Start();
				timerx.Enabled = true;

			}
		}

		void Serial()
		{
			while (true)
			{
				SP.Write("a");
				textBox1.Text += SP.ReadLine();
				textBox1.Text += "\r\n";
			}
		}

		private void groupBox1_Enter(object sender, EventArgs e)
		{

		}

		private void SP_DataReceived(object sender, System.IO.Ports.SerialDataReceivedEventArgs e)
		{
			this.Invoke(new EventHandler(transfere));
		}

		void transfere(object sender, EventArgs e)
		{
			string dado = SP.ReadLine();
			string[] dados = dado.Split(',');
			textBox1.Text += dados[0];
			textBox1.Text += "%\r\n";
			textBox1.Select(textBox1.Text.Length, 0);
			textBox1.ScrollToCaret();

			textBox2.Text += dados[1];
			textBox2.Text += "°C\r\n";
			textBox2.Select(textBox2.Text.Length, 0);
			textBox2.ScrollToCaret();

			textBox3.Text += dados[2];
			textBox3.Text += "°F\r\n";
			textBox3.Select(textBox3.Text.Length, 0);
			textBox3.ScrollToCaret();

			chart1.Series[0].Points.Add(Convert.ToDouble(dados[0]));
			chart1.Series[1].Points.Add(Convert.ToDouble(dados[1]));
			int x = chart1.Series[0].Points.Count;
			if (x > 100)
				chart1.ChartAreas[0].AxisX.ScaleView.Zoom(x - 100, x);
		}

		private void timer1_Tick(object sender, EventArgs e)
		{
			SP.Write("a");

		}

		private void textBox1_TextChanged(object sender, EventArgs e)
		{

		}

		private void label2_Click(object sender, EventArgs e)
		{

		}

		private void Form1_Load(object sender, EventArgs e)
		{

		}

		private void label4_Click(object sender, EventArgs e)
		{

		}

		private void groupBox2_Enter(object sender, EventArgs e)
		{

		}

		private void Form1_FormClosed(object sender, FormClosedEventArgs e)
		{
			SP.Close();
			X.Abort();
			timerx.Enabled = false;
		}

		private void textBox3_TextChanged(object sender, EventArgs e)
		{

		}

		private void ComboBaud_SelectedIndexChanged(object sender, EventArgs e)
		{

		}
	}
}
