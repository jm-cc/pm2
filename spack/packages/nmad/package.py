from spack import *
import platform

class Nmad(AutotoolsPackage):
    """NewMadeleine communicaiton library"""
    homepage = "http://pm2.gforge.inria.fr/nmad/"

    version('trunk', svn='https://scm.gforge.inria.fr/anonscm/svn/pm2/trunk/nmad')

    resource(
        name='building-tools',
        svn='https://scm.gforge.inria.fr/anonscm/svn/padico/PadicoTM/trunk/PadicoTM/building-tools',
        destination='.'
    )
    
    variant('debug', default=False, description='Build in debug mode')
    variant('optimize', default=True, description='Build in optimized mode')
    variant('mpi', default=True, description='Build MadMPI')
    variant('pioman', default=True, description='Build with pioman')

    depends_on('tbx')
    depends_on('puk')
    depends_on('pioman', when='+pioman')
    depends_on('pkgconfig')
    depends_on("expat")
    depends_on('autoconf')

    build_directory = 'nmad-build'
    
    def configure_args(self):
        spec = self.spec

        config_args = [
            '--without-pukabi',
            '--disable-sampling',
            ]
        
        config_args.extend([
            "--%s-debug"         % ('enable' if '+debug'     in spec else 'disable'),
            "--%s-optimize"      % ('enable' if '+optimize' in spec else 'disable'),
        ])

        return config_args
    
    def autoreconf(self, spec, prefix):
        autogen = Executable("./autogen.sh")
        autogen()
            
        
