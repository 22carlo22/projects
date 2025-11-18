library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.STD_LOGIC_ARITH.ALL;
use IEEE.STD_LOGIC_UNSIGNED.ALL;


entity test is
    Port ( clk : in  STD_LOGIC);
end test;

architecture Behavioral of test is
	component CPU_gen
		Port ( 
		clk 		: in  STD_LOGIC;
      rst 		: in  STD_LOGIC;
      trig 		: in  STD_LOGIC;
		-- Interface to the Cache Controller.
      Address 	: out  STD_LOGIC_VECTOR (15 downto 0);
      wr_rd 	: out  STD_LOGIC;
      cs 		: out  STD_LOGIC;
      DOut 		: out  STD_LOGIC_VECTOR (7 downto 0)
		);
	end component;
	
	component cache_control
		Port(	CPUadd : in STD_LOGIC_VECTOR (15 downto 0);
				CPUwr : in STD_LOGIC;
				CPUcs : in STD_LOGIC;
				clk : in STD_LOGIC;
				mux_i0 : in STD_LOGIC_VECTOR (7 downto 0);
				mux_i1 : in STD_LOGIC_VECTOR (7 downto 0);
				dmux_o0 : out STD_LOGIC_VECTOR (7 downto 0);
				dmux_o1 : out STD_LOGIC_VECTOR (7 downto 0);
				CPUrs : out STD_LOGIC;
				SDRAMwr : out STD_LOGIC;
				SDRAMadd : out STD_LOGIC_VECTOR(15 downto 0);
				SDRAMmstrb : out STD_LOGIC);
	end component;
	
	component sdram
		Port ( add : in  STD_LOGIC_VECTOR (15 downto 0);
				  wr : in  STD_LOGIC;
				  mstrb : in  STD_LOGIC;
				  din : in  STD_LOGIC_VECTOR (7 downto 0);
				  dout : out  STD_LOGIC_VECTOR (7 downto 0));
	end component;
	
	component icon
		PORT (
			CONTROL0 : INOUT STD_LOGIC_VECTOR(35 DOWNTO 0)
		);
	end component;
	
	component ila
		PORT (
			CONTROL : INOUT STD_LOGIC_VECTOR(35 DOWNTO 0);
			CLK : IN STD_LOGIC;
			DATA : IN STD_LOGIC_VECTOR(255 DOWNTO 0);
			TRIG0 : IN STD_LOGIC_VECTOR(7 DOWNTO 0)
		);
	end component;
	
	signal control0 : std_logic_vector(35 downto 0);
	signal ila_data : std_logic_vector(255 downto 0);
	signal trig0 : std_logic_vector(7 downto 0);
	
	signal CPUadd : std_logic_vector (15 downto 0);
	signal CPUwr : std_logic;
	signal CPUcs : std_logic;
	signal CPUrdy : std_logic;
	signal CPUdout : std_logic_vector (7 downto 0);
	signal CPUdin : std_logic_vector (7 downto 0);
	
	signal SDRAMadd : std_logic_vector (15 downto 0);
	signal SDRAMwr : std_logic;
	signal SDRAMmstrb : std_logic;
	signal SDRAMdin : std_logic_vector (7 downto 0);
	signal SDRAMdout : std_logic_vector (7 downto 0);
	
	signal i : std_logic_vector(1 downto 0) := "00";
	
begin

	sys_icon : icon port map (
		CONTROL0 => control0
	);
	
	sys_ila : ila port map (
		CONTROL => control0,
		CLK => clk,
		DATA => ila_data,
		TRIG0 => trig0
	);

	sys_cpu : CPU_gen port map(
		clk => i(0),
      rst => '0',
      trig => CPUrdy,
      Address 	=> CPUadd,
      wr_rd => CPUwr,
      cs => CPUcs,
      DOut => CPUdout
	);
	
	sys_cache : cache_control port map(
		CPUadd => CPUadd,
		CPUwr => CPUwr,
		CPUcs => CPUcs,
		clk => i(0),
		mux_i0 => CPUdout,
		mux_i1 => SDRAMdout,
		dmux_o0 => SDRAMdin,
		dmux_o1 => CPUdin,
		CPUrs => CPUrdy,
		SDRAMwr => SDRAMwr,
		SDRAMadd => SDRAMadd,
		SDRAMmstrb => SDRAMmstrb
	);
	
	sys_sdram : sdram port map (
		add => SDRAMadd,
		wr => SDRAMwr,
		mstrb => SDRAMmstrb,
		din => SDRAMdin,
		dout => SDRAMdout
	);
	
	process(clk)
	begin
		if(clk'Event and clk = '1') then
			i <= i + '1';
		end if;
	end process;
	
		ila_data(0) <= i(0);
		ila_data(16 downto 1) <= SDRAMadd;
		ila_data(17) <= SDRAMwr;
		ila_data(18) <= SDRAMmstrb;
		ila_data(26 downto 19) <= SDRAMdin;
		ila_data(34 downto 27) <= SDRAMdout;
		ila_data(50 downto 35) <= CPUadd;
		ila_data(51) <= CPUwr;
		ila_data(52) <= CPUcs ;
		ila_data(60 downto 53) <= CPUdout;
		ila_data(68 downto 61) <= CPUdin;
		ila_data(69) <= CPUrdy;
		ila_data(255 downto 70) <= (others => '0');
	


end Behavioral;
