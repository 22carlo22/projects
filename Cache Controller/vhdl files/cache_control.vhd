library IEEE;
use IEEE.STD_LOGIC_1164.ALL;

entity cache_control is
	Port(		CPUadd : in STD_LOGIC_VECTOR (15 downto 0);
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
end cache_control;

architecture Behavioral of cache_control is
	signal CPUindex : STD_LOGIC_VECTOR (2 downto 0);
	signal CPUtag : STD_LOGIC_VECTOR (7 downto 0);
	signal CPUoffset : STD_LOGIC_VECTOR (4 downto 0);
	
	signal UCinc : STD_LOGIC;
	signal UCout : STD_LOGIC_VECTOR (4 downto 0);
	signal UCbit0 : STD_LOGIC;
	signal UCovf : STD_LOGIC;
	
	signal SRAMwrs: STD_LOGIC_VECTOR (1 downto 0);
	signal SRAMdout : STD_LOGIC;
	signal SRAMdin : STD_LOGIC;
	signal SRAMwen : STD_LOGIC;
	signal SRAMadd : STD_LOGIC_VECTOR(7 downto 0);
	signal SRAMin : STD_LOGIC_VECTOR(7 downto 0);
	signal SRAMout : STD_LOGIC_VECTOR(7 downto 0);
	
	signal SDRAM_wr : STD_LOGIC;
	
	signal MUX5bus_out : STD_LOGIC_VECTOR (4 downto 0);
	signal MUX8bus_out : STD_LOGIC_VECTOR (7 downto 0);
	
	signal TVDwrT : STD_LOGIC;
	signal TVDwrV : STD_LOGIC;
	signal TVDwrD : STD_LOGIC;
	signal TVDinD : STD_LOGIC;
	signal TVDinV : STD_LOGIC;
	signal TVDtag_out : STD_LOGIC_VECTOR (7 downto 0);
	signal TVDhit : STD_LOGIC;
	signal TVDdirty : STD_LOGIC;
	signal TVDvalid : STD_LOGIC;
	
	signal MUX5bus_select : STD_LOGIC;
	signal rs : STD_LOGIC;
	
	component awr
	Port ( addr : in  STD_LOGIC_VECTOR (15 downto 0);
		clk : in STD_LOGIC;
           	index : out  STD_LOGIC_VECTOR (2 downto 0);
           	tag : out  STD_LOGIC_VECTOR (7 downto 0);
           offset : out  STD_LOGIC_VECTOR (4 downto 0));
	end component;
	
	component fsm
	Port ( CPUwr : in  STD_LOGIC;
           CPUcs : in  STD_LOGIC;
           UCovf : in  STD_LOGIC;
           TVDhit : in  STD_LOGIC;
	   clk: in  STD_LOGIC;
	   TVDdirty: in  STD_LOGIC;
           CPUrs : out  STD_LOGIC;
           SRAMdout : out  STD_LOGIC;
           SRAMdin : out  STD_LOGIC;
           SRAMwrs : out  STD_LOGIC_VECTOR (1 downto 0);
           SDRAMwr : out  STD_LOGIC;
           UCinc : out  STD_LOGIC;
           TVDwrT : out  STD_LOGIC;
           TVDwrV : out  STD_LOGIC;
           TVDwrD : out  STD_LOGIC;
           TVDinV : out  STD_LOGIC;
           TVDinD : out  STD_LOGIC;
	   TVDvalid : in STD_LOGIC);
	end component;
	
	component mux2to1_8bus
		Port ( s : in  STD_LOGIC;
           i0 : in  STD_LOGIC_VECTOR (7 downto 0);
           i1 : in  STD_LOGIC_VECTOR (7 downto 0);
           o : out  STD_LOGIC_VECTOR (7 downto 0));
	end component;
	
	component mux2to1
		Port ( s : in  STD_LOGIC;
           i0 : in  STD_LOGIC_VECTOR (4 downto 0);
           i1 : in  STD_LOGIC_VECTOR (4 downto 0);
           o : out  STD_LOGIC_VECTOR (4 downto 0));
	end component;
	
	component muxforSRAMwen
		Port ( s : in  STD_LOGIC_VECTOR (1 downto 0);
           	i1 : in  STD_LOGIC;
		i2 : in  STD_LOGIC;
           	o : out  STD_LOGIC);
	end component;
	
	component counter
		Port ( inc : in  STD_LOGIC;
		clk : in STD_LOGIC;
           	output : out  STD_LOGIC_VECTOR (4 downto 0);
           	bit0 : out  STD_LOGIC;
           	overflow : out  STD_LOGIC);
	end component;
	
	component sram
		Port ( clk : in  STD_LOGIC;
           	wr : in  STD_LOGIC;
           	din : in  STD_LOGIC_VECTOR (7 downto 0);
           	dout : out  STD_LOGIC_VECTOR (7 downto 0);
           	add : in  STD_LOGIC_VECTOR (7 downto 0));
	end component;
	
	component demu1to2_8bus
		Port ( i : in  STD_LOGIC_VECTOR (7 downto 0);
		o0 : out  STD_LOGIC_VECTOR (7 downto 0);
		o1 : out  STD_LOGIC_VECTOR (7 downto 0);
		s : in  STD_LOGIC);
	end component;
	
	component tagvd
		Port ( 		  index : in  STD_LOGIC_VECTOR (2 downto 0);
				  tag_in : in  STD_LOGIC_VECTOR (7 downto 0);
				  tag_wr : in  STD_LOGIC;
				  v_in : in  STD_LOGIC;
				  d_in : in  STD_LOGIC;
				  v_wr : in  STD_LOGIC;
				  d_wr : in  STD_LOGIC;
				  tag_out : out  STD_LOGIC_VECTOR (7 downto 0);
				  hit : out  STD_LOGIC;
				  valid : out STD_LOGIC;
				  dirty : out STD_LOGIC
				  );
	end component;
	
begin
	
	
	sys_awr : awr port map(	addr => CPUadd,
				clk => rs,
				index => CPUindex,
				tag => CPUtag,
				offset => CPUoffset);
								
	sys_tvd : tagvd port map( 	index => CPUindex,
					tag_in => CPUtag,
					tag_wr => TVDwrT,
					v_in => TVDinV,
					d_in => TVDinD,
					v_wr => TVDwrV,
					d_wr => TVDwrD,
					tag_out => TVDtag_out,
					hit => TVDhit,
					valid => TVDvalid,
					dirty => TVDdirty
					);
								
	sys_fsm : fsm port map( CPUwr => CPUwr,
				CPUcs => CPUcs,
				UCovf => UCovf,
				TVDhit => TVDhit,
				clk => clk,
				TVDdirty => TVDdirty,
				CPUrs => rs,
				SRAMdout => SRAMdout,
				SRAMdin => SRAMdin,
				SRAMwrs => SRAMwrs,
				SDRAMwr => SDRAM_wr,
				UCinc => UCinc,
				TVDwrT => TVDwrT,
				TVDwrV => TVDwrV,
				TVDwrD => TVDwrD,
				TVDinV => TVDinV,
				TVDinD => TVDinD,
				TVDvalid => TVDvalid
				);
	
	sys_counter : counter port map( inc => UCinc,
					clk => clk,
					output => UCout,
					bit0 => UCbit0,
					overflow => UCovf);
									  
	sys_mux2to1_8bus : mux2to1_8bus port map(s => SDRAM_wr,
						i0 => CPUtag,
						i1 => TVDtag_out,
						o => MUX8bus_out);
	
	
	MUX5bus_select <= TVDhit and TVDvalid;
	sys_mux2to1_5bus : mux2to1 port map(  	s => MUX5bus_select,
						i0 => UCout,
						i1 => CPUoffset,
						o => MUX5bus_out);
													  

	sys_muxforSRAMwen : muxforSRAMwen port map( 	s => SRAMwrs,
							i1 => UCovf,
							i2 => UCbit0,
							o => SRAMwen);
							SRAMadd(4 downto 0) <= MUX5bus_out;
							SRAMadd(7 downto 5) <= CPUindex;
							SDRAMmstrb <= UCbit0 and not(UCovf);
							SDRAMwr <= SDRAM_wr;
							SDRAMadd(4 downto 0) <= UCout;
							SDRAMadd(7 downto 5) <= CPUindex;
							SDRAMadd(15 downto 8) <= MUX8bus_out;
							CPUrs <= rs;
	
	sys2_mux2to1_8bus : mux2to1_8bus port map(	s => SRAMdin,
							i0 => mux_i0,
							i1 => mux_i1,
							o => SRAMin);
												
	
	sys_sram : sram PORT MAP (	clk => clk,
					wr => SRAMwen,
					din => SRAMin,
					dout => SRAMout,
					add => SRAMadd);
	
	sys_demux1to2_8bus : demu1to2_8bus port map(	i => SRAMout,
							o0 => dmux_o0,
							o1 => dmux_o1,
							s => SRAMdout);

end Behavioral;
